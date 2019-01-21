#include "RaytracerWindows.h"
#include "Core/Util.h"
#include "Core/IHitable.h"
#include "Core/Sphere.h"
#include "Core/MovingSphere.h"
#include "Core/HitableList.h"
#include "Core/HitableTransform.h"
#include "Core/BVHNode.h"
#include "Core/HitableBox.h"
#include "Core/XYZRect.h"
#include "Core/Material.h"
#include "Core/Quat.h"
#include "SampleScenes.h"

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Window.h>

#include <wrl.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace Microsoft::WRL;

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMatrices
{
    XMMATRIX ModelMatrix;
    XMMATRIX ModelViewMatrix;
    XMMATRIX InverseTransposeModelViewMatrix;
    XMMATRIX ModelViewProjectionMatrix;
};

#pragma pack(push, 16) 
struct RenderMaterial
{
    RenderMaterial(
        DirectX::XMFLOAT4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f },
        DirectX::XMFLOAT4 ambient  = { 0.1f, 0.1f, 0.1f, 1.0f },
        DirectX::XMFLOAT4 diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f },
        DirectX::XMFLOAT4 specular = { 1.0f, 1.0f, 1.0f, 1.0f },
        float specularPower = 128.0f
    )
        : Emissive(emissive)
        , Ambient(ambient)
        , Diffuse(diffuse)
        , Specular(specular)
        , SpecularPower(specularPower) {}

    DirectX::XMFLOAT4   Emissive;
    DirectX::XMFLOAT4   Ambient;
    DirectX::XMFLOAT4   Diffuse;
    DirectX::XMFLOAT4   Specular;

    float               SpecularPower;
    uint32_t            Padding[3];
};
#pragma pack(pop)

struct PipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE            RootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT              InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY        PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS                        VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS                        PS;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT      DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS     RTVFormats;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC               SampleDesc;
};

struct RenderSceneNode
{
    IHitable*               Hitable;
    std::shared_ptr<Mesh>   MeshData;
    XMMATRIX                WorldMatrix;
    RenderMaterial          Material;
};

enum RootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB
    MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
    TextureDiffuse,     // Texture2D DiffuseTexture
    NumRootParameters
};


// [register, space]
static const UINT sShaderRegisterParams[NumRootParameters][2] =
{
    { 0, 0 }, // MatricesCB
    { 0, 1 }, // MaterialCB
    { 0, 0 }  // TextureDiffuse
};

// ----------------------------------------------------------------------------------------------------------------------------

static int    sNumSamplesPerRay   = 500;
static int    sMaxScatterDepth    = 50;
static int    sNumThreads         = 8;
static float  sClearColor[]       = { 0.4f, 0.6f, 0.9f, 1.0f };

static const RenderMaterial MaterialWhite =
{
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.1f, 0.1f, 0.1f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    128.0f
};

// ----------------------------------------------------------------------------------------------------------------------------

static XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR Position, FXMVECTOR Direction, FXMVECTOR Up)
{
    assert(!XMVector3Equal(Direction, XMVectorZero()));
    assert(!XMVector3IsInfinite(Direction));
    assert(!XMVector3Equal(Up, XMVectorZero()));
    assert(!XMVector3IsInfinite(Up));

    XMVECTOR R2 = XMVector3Normalize(Direction);

    XMVECTOR R0 = XMVector3Cross(Up, R2);
    R0 = XMVector3Normalize(R0);

    XMVECTOR R1 = XMVector3Cross(R2, R0);

    XMMATRIX M(R0, R1, R2, Position);

    return M;
}

static void XM_CALLCONV ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX viewProjection, RenderMatrices& mat)
{
    mat.ModelMatrix                     = model;
    mat.ModelViewMatrix                 = model * view;
    mat.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelViewMatrix));
    mat.ModelViewProjectionMatrix       = model * viewProjection;
}

static inline XMMATRIX ComputeFinalMatrix(std::vector<DirectX::XMMATRIX>& matrixStack, XMMATRIX innerMatrix)
{
    XMMATRIX final = innerMatrix;
    for (int i = (int)matrixStack.size() - 1; i >= 0; i--)
    {
        final = final * matrixStack[i];
    }

    return final;
}

static inline XMVECTOR ConvertFromVec3(const Vec3& vec)
{
    // Negate Z
    return XMVectorSet(vec.X(), vec.Y(), -vec.Z(), vec.W());
}

static inline Vec3 ConvertFromXMVector(const XMVECTOR& vec)
{
    // Negate Z
    XMFLOAT4 v4;
    XMStoreFloat4(&v4, vec);
    return Vec3(v4.x, v4.y, -v4.z, v4.w);
}

static void UpdateCameras(float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Camera& raytracerCamera, CameraDX12& renderCamera)
{
    // Get ray tracer camera params
    Vec3   lookFrom, lookAt, up;
    float  vertFov, aspect, aperture, focusDist, t0, t1;
    Vec3   clearColor;
    raytracerCamera.GetCameraParams(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);

    // Update render camera look at
    Vec3  diffVec    = (lookAt - lookFrom);
    Vec3  viewDir    = diffVec.MakeUnitVector();
    Vec3  rightDir   = Cross(up, viewDir).MakeUnitVector();
    float upAngle    = mouseDx * -.1f;
    float rightAngle = mouseDy * -.1f;

    // Rotate around up axis
    viewDir  = Quat::RotateVector(viewDir, up, upAngle);
    rightDir = Quat::RotateVector(rightDir, up, upAngle);

    // Rotate around right axis
    viewDir  = Quat::RotateVector(viewDir, rightDir, rightAngle);

    Vec3 cameraTranslate = (viewDir * forwardAmount) + (rightDir * strafeAmount) + (up * upDownAmount);
    lookFrom += cameraTranslate;
    lookAt    = lookFrom + (viewDir * diffVec.Length());

    // Update raytracer camera
    raytracerCamera.Setup(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);

    // Set the render camera
    XMVECTOR cameraPos    = ConvertFromVec3(lookFrom);
    XMVECTOR cameraTarget = ConvertFromVec3(lookAt);
    XMVECTOR cameraUp     = ConvertFromVec3(up);
    renderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    renderCamera.set_Projection(vertFov, aspect, 0.1f, 10000.0f);
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::RaytracerWindows(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , RenderMode(ModePreview)
    , TheRaytracer(nullptr)
    , Scene(nullptr)
    , ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
    , Forward(0)
    , Backward(0)
    , Left(0)
    , Right(0)
    , Up(0)
    , Down(0)
    , MouseDx(0)
    , MouseDy(0)
    , AllowFullscreenToggle(true)
    , ShiftKeyPressed(false)
    , BackbufferWidth(0)
    , BackbufferHeight(0)
{

    XMVECTOR cameraPos    = XMVectorSet(0, 0, 10, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 0, 0, 1);
    XMVECTOR cameraUp     = XMVectorSet(0, 1, 0, 0);
    RenderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::~RaytracerWindows()
{
    
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnResizeRaytracer()
{
    // Is this the first time running?
    if (TheRaytracer == nullptr)
    {
        auto device = Application::Get().GetDevice();

        D3D12_RESOURCE_DESC textureDesc
            = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, BackbufferWidth, BackbufferHeight, 1);

        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&textureResource)));

        CPURaytracerTex.SetTextureUsage(TextureUsage::Albedo);
        CPURaytracerTex.SetD3D12Resource(textureResource);
        CPURaytracerTex.CreateViews();
        CPURaytracerTex.SetName(L"RaytraceSourceTexture");
    }
    else
    {
        // Resize
        CPURaytracerTex.Resize(BackbufferWidth, BackbufferHeight);

        delete TheRaytracer;
        TheRaytracer = nullptr;
    }
    
    // Create the ray tracer
    TheRaytracer = new Raytracer(BackbufferWidth, BackbufferHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::Raytrace(bool enable)
{
    if (TheRaytracer)
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    if (enable)
    {
        if (!TheRaytracer)
        {
            OnResizeRaytracer();
        }

        RenderMode = ModeRaytracer;
        TheRaytracer->BeginRaytrace(RaytracerCamera, Scene);
    }
    else
    {
        RenderMode = ModePreview;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::NextRenderMode()
{
    RenderMode = (RenderingMode)(((int)RenderMode + 1) % (int)MaxRenderModes);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::LoadScene(std::shared_ptr<CommandList> commandList)
{
    const float aspect = (float)BackbufferWidth / (float)BackbufferHeight;

#if 0
    RaytracerCamera = GetCameraForSample(SceneFinal, aspect);
    World = SampleSceneFinal();
#else
    RaytracerCamera = GetCameraForSample(SceneCornell, aspect);
    Scene = SampleSceneCornellBox(false);
#endif

    std::vector<DirectX::XMMATRIX> matrixStack;
    GenerateRenderListFromWorld(commandList, Scene->GetWorld(), RenderSceneList, matrixStack);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::GenerateRenderListFromWorld(std::shared_ptr<CommandList> commandList, const IHitable* currentHead, std::vector<RenderSceneNode*>& outSceneList, std::vector<DirectX::XMMATRIX>& matrixStack)
{
    const std::type_info& tid = typeid(*currentHead);

    if (tid == typeid(HitableList))
    {
        HitableList* hitList = (HitableList*)currentHead;

        IHitable** list = hitList->GetList();
        const int  listSize = hitList->GetListSize();
        for (int i = 0; i < listSize; i++)
        {
            GenerateRenderListFromWorld(commandList, list[i], outSceneList, matrixStack);
        }
    }
    else if (tid == typeid(BVHNode))
    {
        BVHNode* bvhNode = (BVHNode*)currentHead;

        GenerateRenderListFromWorld(commandList, bvhNode->GetLeft(), outSceneList, matrixStack);
        if (bvhNode->GetLeft() != bvhNode->GetRight())
        {
            GenerateRenderListFromWorld(commandList, bvhNode->GetRight(), outSceneList, matrixStack);
        }
    }
    else if (tid == typeid(HitableTranslate))
    {
        HitableTranslate* translateHitable = (HitableTranslate*)currentHead;
        Vec3              offset           = translateHitable->GetOffset();
        XMMATRIX          translation      = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());

        matrixStack.push_back(translation);
        GenerateRenderListFromWorld(commandList, translateHitable->GetHitObject(), outSceneList, matrixStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(HitableRotateY))
    {
        HitableRotateY*   rotateYHitable = (HitableRotateY*)currentHead;
        XMMATRIX          rotation       = XMMatrixRotationY(XMConvertToRadians(-rotateYHitable->GetAngleDegrees()));

        matrixStack.push_back(rotation);
        GenerateRenderListFromWorld(commandList, rotateYHitable->GetHitObject(), outSceneList, matrixStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Sphere) || tid == typeid(MovingSphere))
    {
        Vec3  offset;
        float radius;
        Material* material = nullptr;
        if (tid == typeid(Sphere))
        {
            Sphere* sphere = (Sphere*)currentHead;
            offset   = sphere->GetCenter();
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }
        else if (tid == typeid(MovingSphere))
        {
            MovingSphere* sphere = (MovingSphere*)currentHead;
            offset   = sphere->Center(1.f);
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }

        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode* newNode     = new RenderSceneNode();

        Vec3 color = material->AlbedoValue(0.5f, 0.5f, Vec3(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        newNode->MeshData    = Mesh::CreateSphere(*commandList, radius * 2.f);
        newNode->WorldMatrix = newMatrix;
        newNode->Material    = newMaterial;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(HitableBox))
    {
        HitableBox* box = (HitableBox*)currentHead;

        Vec3 minP, maxP;
        box->GetPoints(minP, maxP);

        Vec3             diff        = maxP - minP;
        Vec3             offset      = minP + (diff * 0.5f);
        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        XMFLOAT3         sideLengths = XMFLOAT3(fabs(diff.X()), fabs(diff.Y()), fabs(diff.Z()));
        RenderSceneNode* newNode     = new RenderSceneNode();

        newNode->MeshData    = Mesh::CreateCube(*commandList, sideLengths);
        newNode->WorldMatrix = newMatrix;
        newNode->Material    = MaterialWhite;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(XYZRect))
    {
        ; // TODO
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RaytracerWindows::LoadContent()
{
    auto device       = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList  = commandQueue->GetCommandList();

    // sRGB formats provide free gamma correction!
    const DXGI_FORMAT backBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multi-sample quality level that can be used for the given back buffer format
    const DXGI_SAMPLE_DESC sampleDesc = Application::Get().GetMultisampleQualityLevels(backBufferFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);


    // -------------------------------------------------------------------
    // Load scene
    // -------------------------------------------------------------------
    LoadScene(commandList);
    commandList->LoadTextureFromFile(PreviewTex, L"RuntimeData\\checker.jpg");    
    

    // -------------------------------------------------------------------
    // Create a root signature
    // -------------------------------------------------------------------
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // Allow input layout and deny unnecessary access to certain pipeline stages
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_DESCRIPTOR_RANGE1 texDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, sShaderRegisterParams[RootParameters::TextureDiffuse][0]);

        CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
        rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(sShaderRegisterParams[RootParameters::MatricesCB][0], sShaderRegisterParams[RootParameters::MatricesCB][1], D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(sShaderRegisterParams[RootParameters::MaterialCB][0], sShaderRegisterParams[RootParameters::MaterialCB][1], D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[RootParameters::TextureDiffuse].InitAsDescriptorTable(1, &texDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
        CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

        RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state for full screen quad rendering
    // -------------------------------------------------------------------
    {
        // Load shaders
        ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0]     = backBufferFormat;

        PipelineStateStream pipelineStateStream;
        pipelineStateStream.RootSignature         = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = depthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = sampleDesc;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&FullscreenPipelineState)));
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state for preview rendering
    // -------------------------------------------------------------------
    {
        // Load shaders
        ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\Preview_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\Preview_PS.cso", &pixelShaderBlob));

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0]     = backBufferFormat;

        PipelineStateStream pipelineStateStream;
        pipelineStateStream.RootSignature         = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = depthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = sampleDesc;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&PreviewPipelineState)));
    }


    // -------------------------------------------------------------------
    // Create a single color buffer and depth buffer
    // -------------------------------------------------------------------
    {
        Texture colorTexture;
        {
            auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat,
                BackbufferWidth, BackbufferHeight,
                1, 1,
                sampleDesc.Count, sampleDesc.Quality,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

            D3D12_CLEAR_VALUE colorClearValue;
            colorClearValue.Format = colorDesc.Format;
            for (int i = 0; i < 4; i++)
            {
                colorClearValue.Color[i] = sClearColor[i];
            }

            colorTexture = Texture(colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"Color Render Target");
        }

        Texture depthTexture;
        {
            auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat,
                BackbufferWidth, BackbufferHeight,
                1, 1,
                sampleDesc.Count, sampleDesc.Quality,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            D3D12_CLEAR_VALUE depthClearValue;
            depthClearValue.Format       = depthDesc.Format;
            depthClearValue.DepthStencil = { 1.0f, 0 };

            depthTexture = Texture(depthDesc, &depthClearValue, TextureUsage::Depth, L"Depth Render Target");
        }

        // Attach the textures to the render target
        RenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
        RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
    }


    // -------------------------------------------------------------------
    // Execute the command queue and wait for it to finish
    // -------------------------------------------------------------------
    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);


    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::UnloadContent()
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);

    if (BackbufferWidth != e.Width || BackbufferHeight != e.Height)
    {
        BackbufferWidth  = GetMax(1, e.Width);
        BackbufferHeight = GetMax(1, e.Height);

        float aspectRatio = BackbufferWidth / (float)BackbufferHeight;
        RenderCamera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);

        Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(BackbufferWidth), static_cast<float>(BackbufferHeight));

        RenderTarget.Resize(BackbufferWidth, BackbufferHeight);

        OnResizeRaytracer();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);

    float speedMultipler = (ShiftKeyPressed ? 64.f : 32.0f) * 16.f;
    float scale          = speedMultipler * float(e.ElapsedTime);
    float forwardAmount  = (Forward - Backward) * scale;
    float strafeAmount   = (Left - Right) * scale;
    float upDownAmount   = (Up - Down) * scale;

    UpdateCameras(forwardAmount, strafeAmount, upDownAmount, MouseDx, MouseDy, RaytracerCamera, RenderCamera);
    MouseDx = 0;
    MouseDy = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList  = commandQueue->GetCommandList();


    // -------------------------------------------------------------------
    // Clear the render targets
    // -------------------------------------------------------------------
    {
        commandList->ClearTexture(RenderTarget.GetTexture(AttachmentPoint::Color0), sClearColor);
        commandList->ClearDepthStencilTexture(RenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }


    // -------------------------------------------------------------------
    // Setup root sig, viewport, and render target
    // -------------------------------------------------------------------
    commandList->SetGraphicsRootSignature(RootSignature);
    commandList->SetViewport(Viewport);
    commandList->SetScissorRect(ScissorRect);
    commandList->SetRenderTarget(RenderTarget);


    // -------------------------------------------------------------------
    // Render preview objects
    // -------------------------------------------------------------------
    if (RenderMode == ModePreview)
    {
        for (int i = 0; i < RenderSceneList.size(); i++)
        {
            XMMATRIX worldMatrix            = RenderSceneList[i]->WorldMatrix;
            XMMATRIX viewMatrix             = RenderCamera.get_ViewMatrix();
            XMMATRIX viewProjectionMatrix   = viewMatrix * RenderCamera.get_ProjectionMatrix();

            RenderMatrices matrices;
            ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);
            commandList->SetPipelineState(PreviewPipelineState);
            commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
            commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, RenderSceneList[i]->Material);
            commandList->SetShaderResourceView(RootParameters::TextureDiffuse, 0, PreviewTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            RenderSceneList[i]->MeshData->Draw(*commandList);
        }
    }


    // -------------------------------------------------------------------
    // Render raytraced frame
    // -------------------------------------------------------------------
    if (RenderMode == ModeRaytracer && TheRaytracer != nullptr)
    {
        // Update texture
        D3D12_SUBRESOURCE_DATA subresource;
        subresource.RowPitch    = 4 * BackbufferWidth;
        subresource.SlicePitch  = subresource.RowPitch * BackbufferHeight;
        subresource.pData       = TheRaytracer->GetOutputBufferRGBA();
        commandList->CopyTextureSubresource(CPURaytracerTex, 0, 1, &subresource);

        // Draw fullscreen quad
        commandList->SetPipelineState(FullscreenPipelineState);
        commandList->SetShaderResourceView(RootParameters::TextureDiffuse, 0, CPURaytracerTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetNullVertexBuffer(0);
        commandList->SetNullIndexBuffer();
        commandList->Draw(3);
    }


    // -------------------------------------------------------------------
    // Execute the command list
    // -------------------------------------------------------------------
    commandQueue->ExecuteCommandList(commandList);


    // -------------------------------------------------------------------
    // Present
    // -------------------------------------------------------------------
    m_pWindow->Present(RenderTarget.GetTexture(AttachmentPoint::Color0));
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);

    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        switch (e.Key)
        {
            case KeyCode::Escape:
            {
                Application::Get().Quit(0);
            }
            break;

            case KeyCode::Enter:
            {
                if (e.Alt)
                {
                    if (AllowFullscreenToggle)
                    {
                        m_pWindow->ToggleFullscreen();
                        AllowFullscreenToggle = false;
                    }
                }
            }
            break;

            case KeyCode::V:
            {
                m_pWindow->ToggleVSync();
            }
            break;

            case KeyCode::R:
            {
                ;
            }
            break;

            case KeyCode::Up:
            case KeyCode::W:
            {
                Forward = 1.0f;
            }
            break;

            case KeyCode::Left:
            case KeyCode::A:
            {
                Left = 1.0f;
            }
            break;

            case KeyCode::Down:
            case KeyCode::S:
            {
                Backward = 1.0f;
            }
            break;

            case KeyCode::Right:
            case KeyCode::D:
            {
                Right = 1.0f;
            }
            break;

            case KeyCode::N:
            {
                NextRenderMode();
            }
            break;

            case KeyCode::Q:
            {
                Down = 1.0f;
            }
            break;

            case KeyCode::E:
            {
                Up = 1.0f;
            }
            break;

            case KeyCode::Space:
            {
                Raytrace(true);
            }
            break;

            case KeyCode::ShiftKey:
            {
                ShiftKeyPressed = true;
            }
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);

    switch (e.Key)
    {
        case KeyCode::Enter:
        {
            if (e.Alt)
            {
                AllowFullscreenToggle = true;
            }
        }
        break;

        case KeyCode::Up:
        case KeyCode::W:
        {
            Forward = 0.0f;
        }
        break;

        case KeyCode::Left:
        case KeyCode::A:
        {
            Left = 0.0f;
        }
        break;

        case KeyCode::Down:
        case KeyCode::S:
        {
            Backward = 0.0f;
        }
        break;

        case KeyCode::Right:
        case KeyCode::D:
        {
            Right = 0.0f;
        }
        break;

        case KeyCode::Q:
        {
            Down = 0.0f;
        }
        break;

        case KeyCode::E:
        {
            Up = 0.0f;
        }
        break;

        case KeyCode::ShiftKey:
        {
            ShiftKeyPressed = false;
        }
        break;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    super::OnMouseButtonPressed(e);

    switch (e.State)
    {
        case KeyCode::LButton:
        {
            NextRenderMode();
        }
        break;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    const float mouseSpeed = 0.1f;

    if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (e.RightButton)
        {
            MouseDx = e.RelX;
            MouseDy = e.RelY;
            Raytrace(false);

        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnMouseWheel(MouseWheelEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        auto fov = RenderCamera.get_FoV();
        fov -= e.WheelDelta;
        fov = Clamp(fov, 12.0f, 90.0f);

        RenderCamera.set_FoV(fov);
    }
}
