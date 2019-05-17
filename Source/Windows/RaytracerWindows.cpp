// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "RaytracerWindows.h"
#include "SampleScenes.h"
#include "ShaderStructs.h"

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

static int          overrideWidth       = 1280;
static int          overrideHeight      = 640;
static int          sNumSamplesPerRay   = 500;
static int          sMaxScatterDepth    = 50;
static int          sNumThreads         = 4;
static float        sVertFov            = 40.f;
static int          sSampleScene        = SceneMesh;
static float        sClearColor[]       = { 0.2f, 0.2f, 0.2f, 1.0f };
static const float  sShadowmapNear      = 0.1f;
static const float  sShadowmapFar       = 1000.f;

// ----------------------------------------------------------------------------------------------------------------------------

#include "SceneConvert.hpp"

// ----------------------------------------------------------------------------------------------------------------------------

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
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER                Rasterizer;
};


enum RootParameters
{
    MatricesCB,
    MaterialCB,
    GlobalDataCB,
    TextureDiffuse,
    TextureShadowmap,
    SpotLights,
    DirLights,

    NumRootParameters
};

// [register, space]
static const uint32_t sShaderRegisterParams[NumRootParameters][2] =
{
    { 0, 0 }, // MatricesCB
    { 0, 1 }, // MaterialCB
    { 1, 0 }, // GlobalLightDataCB
    { 0, 0 }, // TextureDiffuse
    { 1, 0 }, // TextureShadowmap
    { 0, 1 }, // SpotLights
    { 1, 1 }, // DirLights
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

static void XM_CALLCONV SetupSceneConstantBuffer(FXMMATRIX model, CXMMATRIX view, CXMMATRIX projection, RenderMatrices& mat)
{
    XMStoreFloat4x4(&mat.ModelMatrix, XMMatrixTranspose(model));
    XMStoreFloat4x4(&mat.ViewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mat.ProjectionMatrix, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&mat.ModelViewMatrix, XMMatrixTranspose(model * view));
    XMStoreFloat4x4(&mat.InverseTransposeModelViewMatrix, XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(nullptr, model * view))));
    XMStoreFloat4x4(&mat.ModelViewProjectionMatrix, XMMatrixTranspose(model * view * projection));
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::RaytracerWindows(const std::wstring& name, bool vSync)
    : super(name, overrideWidth, overrideHeight, vSync)
    , RenderMode(ModePreview)
    , WireframeViewEnabled(false)
    , TheRaytracer(nullptr)
    , Scene(nullptr)
    , ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(overrideWidth), static_cast<float>(overrideHeight)))
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
    , ShowHelperWindow(true)
    , LoadSceneRequested(false)
    , BackbufferWidth(0)
    , BackbufferHeight(0)
{
    XMVECTOR cameraPos    = XMVectorSet(0, 0, 10, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 0, 0, 1);
    XMVECTOR cameraUp     = XMVectorSet(0, 1, 0, 0);
    RenderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);

    shadowViewProj = new XMMATRIX;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::~RaytracerWindows()
{
    
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished)
{
    if (actuallyFinished)
    {
        WriteImageAndLog(tracer, "RaytracerWindows");
    }
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
    TheRaytracer = new Raytracer(BackbufferWidth, BackbufferHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads, true);

    // Reset the aspect ratio on the scene camera
    if (Scene != nullptr)
    {
        Scene->GetCamera().SetAspect((float)BackbufferWidth / (float)BackbufferHeight);
    }
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
        TheRaytracer->BeginRaytrace(Scene, OnRaytraceComplete);
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
    // Clean up previous scene
    if (Scene != nullptr)
    {
        delete Scene;
        Scene = nullptr;
    }

    for (int i = 0; i < RenderSceneList.size(); i++)
    {
        delete RenderSceneList[i];
    }
    RenderSceneList.clear();
    SpotLightsList.clear();
    DirLightsList.clear();

    // Load the selected scene
    Scene = GetSampleScene(SampleScene(sSampleScene));
    Scene->GetCamera().SetAspect((float)BackbufferWidth / (float)BackbufferHeight);
    Scene->GetCamera().SetFocusDistanceToLookAt();
    sVertFov = Scene->GetCamera().GetVertFov();

    // Generate render list from loaded scene
    std::vector<DirectX::XMMATRIX> matrixStack;
    std::vector<bool> flipNormalStack;
    GenerateRenderListFromWorld(commandList, Scene->GetWorld(), WhiteTex, RenderSceneList, SpotLightsList, matrixStack, flipNormalStack);

    // Generate main shadowmap for the first light that was encountered
    if (SpotLightsList.size() > 0)
    {
        SpotLightsList[0].ShadowmapId = InitShadowmap(ShadowmapWidth, ShadowmapHeight);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RaytracerWindows::LoadContent()
{
    auto device       = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList  = commandQueue->GetCommandList();

    // Set backbuffer, depth and sample formats.
    BackBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    ShadowmapFormat   = DXGI_FORMAT_R32G32B32A32_FLOAT;
    SampleDesc        = Application::Get().GetMultisampleQualityLevels(BackBufferFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);

    // No multisampling for shadowmap
    ShadowmapSampleDesc.Count   = 1;
    ShadowmapSampleDesc.Quality = 0;

    // Large shadowmap resolution
    ShadowmapWidth  = 2048;
    ShadowmapHeight = 2048;


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

        CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];

        rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(
            sShaderRegisterParams[RootParameters::MatricesCB][0], 
            sShaderRegisterParams[RootParameters::MatricesCB][1], 
            D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
            D3D12_SHADER_VISIBILITY_VERTEX);

        rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(
            sShaderRegisterParams[RootParameters::MaterialCB][0], 
            sShaderRegisterParams[RootParameters::MaterialCB][1], 
            D3D12_ROOT_DESCRIPTOR_FLAG_NONE, 
            D3D12_SHADER_VISIBILITY_PIXEL);

        rootParameters[RootParameters::GlobalDataCB].InitAsConstants(
            sizeof(GlobalData) / 4, 
            sShaderRegisterParams[RootParameters::GlobalDataCB][0],
            sShaderRegisterParams[RootParameters::GlobalDataCB][1],
            D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_DESCRIPTOR_RANGE1 diffuseTexDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, sShaderRegisterParams[RootParameters::TextureDiffuse][0]);
        rootParameters[RootParameters::TextureDiffuse].InitAsDescriptorTable(1, &diffuseTexDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_DESCRIPTOR_RANGE1 shadowTexDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, sShaderRegisterParams[RootParameters::TextureShadowmap][0]);
        rootParameters[RootParameters::TextureShadowmap].InitAsDescriptorTable(1, &shadowTexDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

        rootParameters[RootParameters::SpotLights].InitAsShaderResourceView(
            sShaderRegisterParams[RootParameters::SpotLights][0],
            sShaderRegisterParams[RootParameters::SpotLights][1],
            D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

        rootParameters[RootParameters::DirLights].InitAsShaderResourceView(
            sShaderRegisterParams[RootParameters::DirLights][0],
            sShaderRegisterParams[RootParameters::DirLights][1],
            D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

        
        CD3DX12_STATIC_SAMPLER_DESC samplers[] =
        {
            CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR),
            CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            CD3DX12_STATIC_SAMPLER_DESC(2, D3D12_FILTER_ANISOTROPIC)
        };
        const int numSamplers = sizeof(samplers) / sizeof(CD3DX12_STATIC_SAMPLER_DESC);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, numSamplers, samplers, rootSignatureFlags);

        RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state for full screen quad rendering
    // -------------------------------------------------------------------
    {
        // Load shaders
        ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0]     = BackBufferFormat;

        PipelineStateStream pipelineStateStream;
        pipelineStateStream.RootSignature         = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = DepthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = SampleDesc;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&FullscreenPipelineState)));
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state for preview rendering
    // -------------------------------------------------------------------
    {
        // Load shaders
        ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Preview_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Preview_PS.cso", &pixelShaderBlob));

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0]     = BackBufferFormat;

        PipelineStateStream pipelineStateStream;
        pipelineStateStream.RootSignature         = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = DepthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = SampleDesc;

        // Preview
        {
            // Set rasterizer state
            CD3DX12_RASTERIZER_DESC rasterState;
            ZeroMemory(&rasterState, sizeof(CD3DX12_RASTERIZER_DESC));
            rasterState.FillMode = D3D12_FILL_MODE_SOLID;
            rasterState.CullMode = D3D12_CULL_MODE_NONE;
            pipelineStateStream.Rasterizer = rasterState;

            D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
            ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&PreviewPipelineState)));
        }

        // Wireframe preview 
        {
            // Set rasterizer state
            CD3DX12_RASTERIZER_DESC rasterState;
            ZeroMemory(&rasterState, sizeof(CD3DX12_RASTERIZER_DESC));
            rasterState.FillMode = D3D12_FILL_MODE_WIREFRAME;
            rasterState.CullMode = D3D12_CULL_MODE_NONE;
            pipelineStateStream.Rasterizer = rasterState;

            D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
            ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&WireframePreviewPipelineState)));
        }
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state for shadowmap rendering
    // -------------------------------------------------------------------
    {
        // Load shaders
        ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Shadowmap_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Shadowmap_PS.cso", &pixelShaderBlob));

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = ShadowmapFormat;

        PipelineStateStream pipelineStateStream;
        pipelineStateStream.RootSignature         = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = DepthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = ShadowmapSampleDesc;

        // Set rasterizer state
        CD3DX12_RASTERIZER_DESC rasterState;
        ZeroMemory(&rasterState, sizeof(CD3DX12_RASTERIZER_DESC));
        rasterState.FillMode = D3D12_FILL_MODE_SOLID;
        rasterState.CullMode = D3D12_CULL_MODE_BACK;
        pipelineStateStream.Rasterizer = rasterState;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&ShadowmapPipelineState)));
    }


    // -------------------------------------------------------------------
    // Create a single color buffer and depth buffer
    // -------------------------------------------------------------------
    {
        Texture colorTexture;
        {
            auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(BackBufferFormat,
                BackbufferWidth, BackbufferHeight,
                1, 1,
                SampleDesc.Count, SampleDesc.Quality,
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
            auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DepthBufferFormat,
                BackbufferWidth, BackbufferHeight,
                1, 1,
                SampleDesc.Count, SampleDesc.Quality,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            D3D12_CLEAR_VALUE depthClearValue;
            depthClearValue.Format       = depthDesc.Format;
            depthClearValue.DepthStencil = { 1.0f, 0 };

            depthTexture = Texture(depthDesc, &depthClearValue, TextureUsage::Depth, L"Depth Render Target");
        }

        // Attach the textures to the render target
        DisplayRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
        DisplayRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
    }


    // -------------------------------------------------------------------
    // Load scene
    // -------------------------------------------------------------------
    LoadScene(commandList);
    commandList->LoadTextureFromFile(PreviewTex, RUNTIMEDATA_DIR L"\\checker.jpg");
    commandList->LoadTextureFromFile(WhiteTex, RUNTIMEDATA_DIR L"\\white.png");
    

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

        DisplayRenderTarget.Resize(BackbufferWidth, BackbufferHeight);

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

    UpdateCameras(sVertFov, forwardAmount, strafeAmount, upDownAmount, MouseDx, MouseDy, Scene->GetCamera(), RenderCamera);
    MouseDx = 0;
    MouseDy = 0;

    // Update viewspace positions of lights
    XMMATRIX viewMatrix = RenderCamera.get_ViewMatrix();
    for (int i = 0; i < (int)SpotLightsList.size(); i++)
    {
        SpotLight& light = SpotLightsList[i];

        XMVECTOR positionWS = XMLoadFloat4(&light.PositionWS);
        XMVECTOR positionVS = XMVector3TransformCoord(positionWS, viewMatrix);
        XMStoreFloat4(&light.PositionVS, positionVS);

        XMVECTOR directionWS = XMLoadFloat4(&light.DirectionWS);
        XMVECTOR directionVS = XMVector3Normalize(XMVector3TransformNormal(directionWS, viewMatrix));
        XMStoreFloat4(&light.DirectionVS, directionVS);
    }

    for (int i = 0; i < (int)DirLightsList.size(); i++)
    {
        DirLight& light = DirLightsList[i];

        XMVECTOR directionWS = XMLoadFloat4(&light.DirectionWS);
        XMVECTOR directionVS = XMVector3Normalize(XMVector3TransformNormal(directionWS, viewMatrix));
        XMStoreFloat4(&light.DirectionVS, directionVS);
    }
}
// ----------------------------------------------------------------------------------------------------------------------------

int RaytracerWindows::InitShadowmap(int width, int height)
{
    // Create shadow map
    Texture shadowmapTexture;
    {
        auto shadowmapDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            ShadowmapFormat,
            width, height,
            1, 1,
            ShadowmapSampleDesc.Count, ShadowmapSampleDesc.Quality,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        D3D12_CLEAR_VALUE colorClearValue;
        colorClearValue.Format = shadowmapDesc.Format;
        for (int i = 0; i < 4; i++)
        {
            colorClearValue.Color[i] = sClearColor[i];
        }

        shadowmapTexture = Texture(shadowmapDesc, &colorClearValue, TextureUsage::RenderTarget, L"Shadowmap Rendertarget");
    }

    Texture depthTexture;
    {
        auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DepthBufferFormat,
            width, height,
            1, 1,
            ShadowmapSampleDesc.Count, ShadowmapSampleDesc.Quality,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        D3D12_CLEAR_VALUE depthClearValue;
        depthClearValue.Format = depthDesc.Format;
        depthClearValue.DepthStencil = { 1.0f, 0 };

        depthTexture = Texture(depthDesc, &depthClearValue, TextureUsage::Depth, L"Shadowmap Depth Render Target");
    }

    RenderTarget* shadowmapRT = new RenderTarget();
    shadowmapRT->AttachTexture(AttachmentPoint::Color0, shadowmapTexture);
    shadowmapRT->AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
    ShadowmapRTList.push_back(shadowmapRT);

    return int(ShadowmapRTList.size() - 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::RenderShadowmaps(std::shared_ptr<CommandList>& commandList)
{
    float           clearColor[] = { 0.f, 0.f, 0.f, 0.f };
    D3D12_VIEWPORT  viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, float(ShadowmapWidth), float(ShadowmapHeight), 0.f, 1.f));
    D3D12_RECT      scissorRect(CD3DX12_RECT(0, 0, ShadowmapWidth, ShadowmapHeight));

    for (int i = 0; i < SpotLightsList.size(); i++)
    {
        SpotLight&      light    = SpotLightsList[i];
        RenderTarget*   shadowRT = ShadowmapRTList[light.ShadowmapId];

        // Setup rendering to the shadowmap RT
        commandList->ClearTexture(shadowRT->GetTexture(AttachmentPoint::Color0), clearColor);
        commandList->ClearDepthStencilTexture(shadowRT->GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
        commandList->SetGraphicsRootSignature(RootSignature);
        commandList->SetViewport(viewport);
        commandList->SetScissorRect(scissorRect);
        commandList->SetRenderTarget(*shadowRT);
        commandList->SetPipelineState(ShadowmapPipelineState);

        // Setup light matrices
        float    fovY        = RT_PI / 2.0f;
        float    aspect      = 1.0f;
        XMVECTOR lookFrom    = XMLoadFloat4(&light.SmapWS);
        XMVECTOR lookAt      = XMLoadFloat4(&light.LookAtWS);
        XMVECTOR up          = XMLoadFloat4(&light.UpWS);
        XMMATRIX viewMatrix  = XMMatrixLookAtLH(lookFrom, lookAt, up);
        //XMMATRIX projMatrix  = XMMatrixPerspectiveFovLH(fovY, aspect, sShadowmapNear, sShadowmapFar);
        XMMATRIX projMatrix = XMMatrixOrthographicLH((float)ShadowmapWidth, (float)ShadowmapHeight, sShadowmapNear, sShadowmapFar);

        // Cache the shadow mvp
        *shadowViewProj = viewMatrix * projMatrix;

        GlobalData globalData;
        globalData.NumSpotLights  = 0;
        globalData.NumDirLights   = 0;
        globalData.ShadowmapDepth = sShadowmapFar;
        commandList->SetGraphics32BitConstants(RootParameters::GlobalDataCB, globalData);

        // Render the objects
        RenderPreviewObjects(commandList, viewMatrix, projMatrix, true);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::RenderPreviewObjects(std::shared_ptr<CommandList>& commandList, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool shadowmapRender)
{
    for (int i = 0; i < RenderSceneList.size(); i++)
    {
        // Don't render light shapes
        if (shadowmapRender && RenderSceneList[i]->Hitable->IsALightShape())
        {
            continue;
        }

        RenderMatrices matrices;
        SetupSceneConstantBuffer(RenderSceneList[i]->WorldMatrix, viewMatrix, projectionMatrix, matrices);

        // Prepare shadowmap transform
        if (!shadowmapRender)
        {
            // Matrix to transform from [-1,1]->[0,1]
            XMMATRIX t(
                0.5f, 0.0f, 0.0f, 0.0f,
                0.0f,-0.5f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.5f, 0.5f, 0.0f, 1.0f);

            XMStoreFloat4x4(&matrices.ShadowViewProj, XMMatrixTranspose((*shadowViewProj) * t));
        }

        const Texture& diffuseTex   = (RenderSceneList[i]->DiffuseTexture != nullptr) ? *RenderSceneList[i]->DiffuseTexture : WhiteTex;
        const Texture& shadowmapTex = (!shadowmapRender && ShadowmapRTList.size() > 0) ? ShadowmapRTList[0]->GetTexture(Color0) : WhiteTex;

        commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
        commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, RenderSceneList[i]->Material);
        commandList->SetShaderResourceView(RootParameters::TextureDiffuse, 0, diffuseTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(RootParameters::TextureShadowmap, 0, shadowmapTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        RenderSceneList[i]->MeshData->Draw(*commandList);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnGUI()
{
    if (!ShowHelperWindow)
        return;

    ImGui::SetNextWindowPos(ImVec2(770, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 460), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags window_flags = 0;
    if (!ImGui::Begin("Raytracer", &ShowHelperWindow, window_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::Separator();
    ImGui::Text("*** STATUS ***");
    ImGui::Separator();
    ImGui::BulletText("Render Mode: %s", RenderingModeStrings[RenderMode]);

    if (TheRaytracer != nullptr && TheRaytracer->IsTracing())
    {
        {
            Raytracer::Stats stats = TheRaytracer->GetStats();
            double percentage = double(double(stats.NumPixelSamples) / double(stats.TotalNumPixelSamples));
            int    percentInt = (int)(percentage * 100);
            int    numMinutes = stats.TotalTimeInSeconds / 60;
            int    numSeconds = stats.TotalTimeInSeconds % 60;

            ImGui::BulletText("Completion: %d%%", percentInt);
            ImGui::BulletText("Time: %dm:%ds", numMinutes, numSeconds);
            ImGui::BulletText("Resolution: %dx%d", BackbufferWidth, BackbufferHeight);
            ImGui::BulletText("Scatter Depth: %d", sMaxScatterDepth);
            ImGui::BulletText("Num Threads: %d", sNumThreads);
            ImGui::BulletText("Num Samples: %d", sNumSamplesPerRay);
            ImGui::BulletText("Done Samples: %d", stats.CompletedSampleCount);
            ImGui::BulletText("Rays Fired: %" PRId64 "", stats.TotalRaysFired);
            ImGui::BulletText("Num Pixels Sampled: %" PRId64 "", stats.NumPixelSamples);
            ImGui::BulletText("Pdf Query Retries: %d", stats.NumPdfQueryRetries);
        }

        ImGui::Text("");
        if (ImGui::Button("Stop Trace", ImVec2(100, 50)))
        {
            Raytrace(false);
        }
    }
    else
    {
        char stringBuf[128];
        bool rayTracerDirty = false;

        ImGui::Separator();
        ImGui::Text("*** HELP ***");
        ImGui::Separator();
        ImGui::BulletText("Translate camera:    Press keys WASD strafe, QE up and down");
        ImGui::BulletText("Pan camera:          Right mouse button (press and hold)");
        ImGui::BulletText("Fov adjust:          Mouse wheel");
        ImGui::BulletText("Start/stop trace:    Space bar/escape key");
        ImGui::BulletText("Cycle render modes:  R key");
        ImGui::BulletText("Toggle wireframe:    F key");


        ImGui::Separator();
        ImGui::Text("*** RAYTRACING ***");
        ImGui::Separator();
        if (ImGui::ListBox("Scene Select", &sSampleScene, SampleSceneNames, IM_ARRAYSIZE(SampleSceneNames), MaxScene))
        {
            LoadSceneRequested = true;
        }

        _itoa_s(sNumSamplesPerRay, stringBuf, 10);
        if(ImGui::InputText("Num Samples", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
        {
            sNumSamplesPerRay = atoi(stringBuf);
            rayTracerDirty = true;
        }

        _itoa_s(sMaxScatterDepth, stringBuf, 10);
        if (ImGui::InputText("Scatter Depth", stringBuf, IM_ARRAYSIZE(stringBuf), ImGuiInputTextFlags_CharsDecimal))
        {
            sMaxScatterDepth = atoi(stringBuf);
            rayTracerDirty = true;
        }

        if (ImGui::SliderInt("Num Threads", &sNumThreads, 1, 32))
        {
            rayTracerDirty = true;
        }

        // If any options changed, recreate the raytracer
        if (rayTracerDirty)
        {
            OnResizeRaytracer();
        }

        ImGui::Text("");
        if (ImGui::Button("Begin Trace", ImVec2(100, 50)))
        {
            Raytrace(true);
        }
    }
    

    ImGui::End();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList  = commandQueue->GetCommandList();

    if (LoadSceneRequested)
    {
        LoadScene(commandList);
        LoadSceneRequested = false;
    }

    // -------------------------------------------------------------------
    // Render shadowmaps first
    // -------------------------------------------------------------------
    RenderShadowmaps(commandList);


    // -------------------------------------------------------------------
    // Clear the render targets
    // -------------------------------------------------------------------
    {
        commandList->ClearTexture(DisplayRenderTarget.GetTexture(AttachmentPoint::Color0), sClearColor);
        commandList->ClearDepthStencilTexture(DisplayRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }


    // -------------------------------------------------------------------
    // Setup root sig, viewport, and render target
    // -------------------------------------------------------------------
    commandList->SetGraphicsRootSignature(RootSignature);
    commandList->SetViewport(Viewport);
    commandList->SetScissorRect(ScissorRect);
    commandList->SetRenderTarget(DisplayRenderTarget);


    // -------------------------------------------------------------------
    // Render preview objects
    // -------------------------------------------------------------------
    if (RenderMode == ModePreview)
    {
        // Upload lights
        GlobalData globalData;
        globalData.NumSpotLights = int(SpotLightsList.size());
        globalData.NumDirLights  = int(DirLightsList.size());
        commandList->SetGraphics32BitConstants(RootParameters::GlobalDataCB, globalData);
        commandList->SetGraphicsDynamicStructuredBuffer(RootParameters::SpotLights, SpotLightsList);
        commandList->SetGraphicsDynamicStructuredBuffer(RootParameters::DirLights, DirLightsList);

        // Preview rendering pipeline state
        commandList->SetPipelineState(WireframeViewEnabled ? WireframePreviewPipelineState : PreviewPipelineState);

        // Render objects
        RenderPreviewObjects(commandList, RenderCamera.get_ViewMatrix(), RenderCamera.get_ProjectionMatrix(), false);
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
        commandList->SetShaderResourceView(RootParameters::TextureShadowmap, 0, WhiteTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetNullVertexBuffer(0);
        commandList->SetNullIndexBuffer();
        commandList->Draw(3);
    }

    // -------------------------------------------------------------------
    // Render GUI
    // -------------------------------------------------------------------
    OnGUI();

    // -------------------------------------------------------------------
    // Execute the command list
    // -------------------------------------------------------------------
    commandQueue->ExecuteCommandList(commandList);


    // -------------------------------------------------------------------
    // Present
    // -------------------------------------------------------------------
    m_pWindow->Present(DisplayRenderTarget.GetTexture(AttachmentPoint::Color0));
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
                Raytrace(false);
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
                NextRenderMode();
            }
            break;

            case KeyCode::F:
            {
                WireframeViewEnabled = !WireframeViewEnabled;
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
                ShowHelperWindow = true;
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

    if (e.RightButton)
    {
        RenderMode = ModePreview;
        ShowHelperWindow = false;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    super::OnMouseButtonReleased(e);

    if (e.Button == MouseButtonEventArgs::Right)
    {
        ShowHelperWindow = true;
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
        sVertFov -= e.WheelDelta;
        sVertFov = Clamp(sVertFov, 12.0f, 90.0f);
        Raytrace(false);
    }
}
