#include "RaytracerWindows.h"
#include "Core/Util.h"

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

struct Mat
{
    XMMATRIX ModelMatrix;
    XMMATRIX ModelViewMatrix;
    XMMATRIX InverseTransposeModelViewMatrix;
    XMMATRIX ModelViewProjectionMatrix;
};

enum RootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB
    Textures,           // Texture2D DiffuseTexture
    NumRootParameters
};

static const UINT sRootParamRegisters[NumRootParameters] =
{
    0, // MatricesCB
    0, // Textures
};

static FLOAT sClearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

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

static void XM_CALLCONV ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX viewProjection, Mat& mat)
{
    mat.ModelMatrix                     = model;
    mat.ModelViewMatrix                 = model * view;
    mat.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelViewMatrix));
    mat.ModelViewProjectionMatrix       = model * viewProjection;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::RaytracerWindows(Raytracer* tracer, const Camera& cam, IHitable* world, const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , Tracer(tracer)
    , RaytracerCamera(cam)
    , World(world)
    , ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
    , Forward(0)
    , Backward(0)
    , Left(0)
    , Right(0)
    , Up(0)
    , Down(0)
    , Pitch(0)
    , Yaw(0)
    , AllowFullscreenToggle(true)
    , ShiftKeyPressed(false)
    , Width(0)
    , Height(0)
{

    XMVECTOR cameraPos    = XMVectorSet(0, 5, -20, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp     = XMVectorSet(0, 1, 0, 0);
    RenderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);

    PtrAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
    PtrAlignedCameraData->InitialCamPos = RenderCamera.get_Translation();
    PtrAlignedCameraData->InitialCamRot = RenderCamera.get_Rotation();
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracerWindows::~RaytracerWindows()
{
    _aligned_free(PtrAlignedCameraData);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RaytracerWindows::LoadContent()
{
    // Cache these for easy reference
    auto device       = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList  = commandQueue->GetCommandList();

    // sRGB formats provide free gamma correction!
    const DXGI_FORMAT backBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multi-sample quality level that can be used for the given back buffer format
    const DXGI_SAMPLE_DESC sampleDesc = Application::Get().GetMultisampleQualityLevels(backBufferFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);

#if 1
    D3D12_RESOURCE_DESC textureDesc
        = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(512), static_cast<UINT>(512), static_cast<UINT16>(1));;

    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textureResource)));

    CPURaytracerFrame.SetTextureUsage(TextureUsage::Albedo);
    CPURaytracerFrame.SetD3D12Resource(textureResource);
    CPURaytracerFrame.CreateViews();
    CPURaytracerFrame.SetName(L"RaytraceSourceTexture");

#endif
    

    // -------------------------------------------------------------------
    // Load shaders
    // -------------------------------------------------------------------
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    {
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\VertexShader.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(L"RuntimeData\\PixelShader.cso", &pixelShaderBlob));
    }


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

        CD3DX12_DESCRIPTOR_RANGE1 texDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, sRootParamRegisters[RootParameters::Textures]);

        CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
        rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &texDescRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
        CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

        RootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);
    }


    // -------------------------------------------------------------------
    // Setup the pipeline state
    // -------------------------------------------------------------------
    {
        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0]     = backBufferFormat;

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
            CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        } pipelineStateStream;

        pipelineStateStream.pRootSignature        = RootSignature.GetRootSignature().Get();
        pipelineStateStream.InputLayout           = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        pipelineStateStream.DSVFormat             = depthBufferFormat;
        pipelineStateStream.RTVFormats            = rtvFormats;
        pipelineStateStream.SampleDesc            = sampleDesc;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
        ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&PipelineState)));
    }


    // -------------------------------------------------------------------
    // Create a single color buffer and depth buffer
    // -------------------------------------------------------------------
    {
        Texture colorTexture;
        {
            auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat,
                Width, Height,
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
                Width, Height,
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

void RaytracerWindows::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);

    if (Width != e.Width || Height != e.Height)
    {
        Width  = GetMax(1, e.Width);
        Height = GetMax(1, e.Height);

        float aspectRatio = Width / (float)Height;
        RenderCamera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);

        Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height));

        RenderTarget.Resize(Width, Height);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::UnloadContent()
{
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);

    // Update the camera
    float    speedMultipler  = (ShiftKeyPressed ? 16.0f : 4.0f);
    XMVECTOR cameraTranslate = XMVectorSet(Right - Left, 0.0f, Forward - Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    XMVECTOR cameraPan       = XMVectorSet(0.0f, Up - Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    XMVECTOR cameraRotation  = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(Pitch), XMConvertToRadians(Yaw), 0.0f);

    RenderCamera.Translate(cameraTranslate, Space::Local);
    RenderCamera.Translate(cameraPan, Space::Local);
    RenderCamera.set_Rotation(cameraRotation);    
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracerWindows::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    // Cache these for easy reference
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList  = commandQueue->GetCommandList();

    // Clear the render targets
    {
        commandList->ClearTexture(RenderTarget.GetTexture(AttachmentPoint::Color0), sClearColor);
        commandList->ClearDepthStencilTexture(RenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }

    // Setup pipeline, root sig, viewport, and render target
    commandList->SetPipelineState(PipelineState);
    commandList->SetGraphicsRootSignature(RootSignature);
    commandList->SetViewport(Viewport);
    commandList->SetScissorRect(ScissorRect);
    commandList->SetRenderTarget(RenderTarget);

    // Update raytraced frame
    D3D12_SUBRESOURCE_DATA subresource;
    subresource.RowPitch    = 4 * 512;
    subresource.SlicePitch  = subresource.RowPitch * 512;
    subresource.pData       = Tracer->GetOutputBufferRGBA();
    commandList->CopyTextureSubresource(CPURaytracerFrame, 0, static_cast<uint32_t>(1), &subresource);

    // Execute the command list
    commandQueue->ExecuteCommandList(commandList);

    // Present the frame
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
                // Reset camera transform
                RenderCamera.set_Translation(PtrAlignedCameraData->InitialCamPos);
                RenderCamera.set_Rotation(PtrAlignedCameraData->InitialCamRot);
                Pitch = 0.0f;
                Yaw = 0.0f;
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

void RaytracerWindows::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    const float mouseSpeed = 0.1f;

    if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (e.LeftButton)
        {
            Pitch -= e.RelY * mouseSpeed;
            Pitch  = Clamp(Pitch, -90.0f, 90.0f);
            Yaw   -= e.RelX * mouseSpeed;
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
