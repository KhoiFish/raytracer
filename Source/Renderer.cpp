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

#include "Renderer.h"
#include "RealtimeEngine/RenderDevice.h"
#include "RealtimeEngine/PlatformApp.h"
#include "RealtimeEngine/CommandList.h"
#include "RealtimeEngine/CommandContext.h"
#include <d3dcompiler.h>
#include "Windows/SceneConvert.hpp"

using namespace Core;
using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

static int     sNumSamplesPerRay = 500;
static int     sMaxScatterDepth  = 50;
static int     sNumThreads       = 4;
static float   sVertFov          = 40.f;
static int     sSampleScene      = SceneCornellSmoke;
static float   sClearColor[]     = { 0.2f, 0.2f, 0.2f, 1.0f };

const D3D12_INPUT_ELEMENT_DESC RenderVertex::InputElements[] =
{
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

// ----------------------------------------------------------------------------------------------------------------------------

enum FullscreenQuadRootIndex
{
    FullscreenQuadRootIndex_Texture0 = 0,

    FullscreenQuadRootIndex_Num
};

enum FullscreenQuadRegisters
{
    FullscreenQuadRegisters_Texture0      = 0,
    FullscreenQuadRegisters_LinearSampler = 0
};

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface(width, height)
    , RenderMode(RenderingMode_Cpu)
    , TheRaytracer(nullptr)
    , TheWorldScene(nullptr)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::~Renderer()
{
    if (TheRaytracer != nullptr)
    {
        delete TheRaytracer;
    }

    if (TheWorldScene != nullptr)
    {
        delete TheWorldScene;
        TheWorldScene = nullptr;
    }

    for (int i = 0; i < TheRealtimeSceneList.size(); i++)
    {
        delete TheRealtimeSceneList[i];
    }
    TheRealtimeSceneList.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceLost()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceRestored()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnInit()
{
    // Get the number of cores on this system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    sNumThreads = sysInfo.dwNumberOfProcessors;

    // Init the render device
    RenderDevice::Initialize(PlatformApp::GetHwnd(), Width, Height, this);

    // Setup render pipelines
    SetupFullscreenQuadPipeline();
    SetupRealtimePipeline();

    // Load the scene data
    LoadScene();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupFullscreenQuadPipeline()
{
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode                                 = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode                                 = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise                    = TRUE;
    rasterizerDesc.DepthBias                                = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp                           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias                     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable                          = TRUE;
    rasterizerDesc.MultisampleEnable                        = FALSE;
    rasterizerDesc.AntialiasedLineEnable                    = FALSE;
    rasterizerDesc.ForcedSampleCount                        = 0;
    rasterizerDesc.ConservativeRaster                       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDisable = {};
    blendDisable.IndependentBlendEnable                     = FALSE;
    blendDisable.RenderTarget[0].BlendEnable                = FALSE;
    blendDisable.RenderTarget[0].SrcBlend                   = D3D12_BLEND_SRC_ALPHA;
    blendDisable.RenderTarget[0].DestBlend                  = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOp                    = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].SrcBlendAlpha              = D3D12_BLEND_ONE;
    blendDisable.RenderTarget[0].DestBlendAlpha             = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOpAlpha               = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].RenderTargetWriteMask      = 0;
    blendDisable.RenderTarget[0].RenderTargetWriteMask      = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStateReadWrite;
    depthStateReadWrite.DepthEnable                         = FALSE;
    depthStateReadWrite.DepthWriteMask                      = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStateReadWrite.DepthFunc                           = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    depthStateReadWrite.StencilEnable                       = FALSE;
    depthStateReadWrite.StencilReadMask                     = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateReadWrite.StencilWriteMask                    = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateReadWrite.FrontFace.StencilFunc               = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateReadWrite.FrontFace.StencilPassOp             = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilFailOp             = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilDepthFailOp        = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.BackFace                            = depthStateReadWrite.FrontFace;

    D3D12_SAMPLER_DESC defaultSamplerDesc;
    defaultSamplerDesc.Filter                               = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    defaultSamplerDesc.AddressU                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.AddressV                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.AddressW                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.MipLODBias                           = 0.0f;
    defaultSamplerDesc.MaxAnisotropy                        = 8;
    defaultSamplerDesc.ComparisonFunc                       = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    defaultSamplerDesc.BorderColor[0]                       = 1.0f;
    defaultSamplerDesc.BorderColor[1]                       = 1.0f;
    defaultSamplerDesc.BorderColor[2]                       = 1.0f;
    defaultSamplerDesc.BorderColor[3]                       = 1.0f;
    defaultSamplerDesc.MinLOD                               = 0.0f;
    defaultSamplerDesc.MaxLOD                               = D3D12_FLOAT32_MAX;

    // Fullscreen quad signature setup
    {
        const int numSamplers = 1;
        FullscreenQuadRootSignature.Reset(FullscreenQuadRootIndex_Num, numSamplers);
        FullscreenQuadRootSignature[FullscreenQuadRootIndex_Texture0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, FullscreenQuadRegisters_Texture0, 1);
        FullscreenQuadRootSignature.InitStaticSampler(FullscreenQuadRegisters_LinearSampler, defaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        FullscreenQuadRootSignature.Finalize("FullscreenQuadRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // Full screen pipeline state setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        FullscreenPipelineState.SetRootSignature(FullscreenQuadRootSignature);
        FullscreenPipelineState.SetRasterizerState(rasterizerDesc);
        FullscreenPipelineState.SetBlendState(blendDisable);
        FullscreenPipelineState.SetDepthStencilState(depthStateReadWrite);
        FullscreenPipelineState.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        FullscreenPipelineState.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        FullscreenPipelineState.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        FullscreenPipelineState.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        FullscreenPipelineState.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        FullscreenPipelineState.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRealtimePipeline()
{
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode                                 = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode                                 = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise                    = TRUE;
    rasterizerDesc.DepthBias                                = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp                           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias                     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable                          = TRUE;
    rasterizerDesc.MultisampleEnable                        = FALSE;
    rasterizerDesc.AntialiasedLineEnable                    = FALSE;
    rasterizerDesc.ForcedSampleCount                        = 0;
    rasterizerDesc.ConservativeRaster                       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDisable = {};
    blendDisable.IndependentBlendEnable                     = FALSE;
    blendDisable.RenderTarget[0].BlendEnable                = FALSE;
    blendDisable.RenderTarget[0].SrcBlend                   = D3D12_BLEND_SRC_ALPHA;
    blendDisable.RenderTarget[0].DestBlend                  = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOp                    = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].SrcBlendAlpha              = D3D12_BLEND_ONE;
    blendDisable.RenderTarget[0].DestBlendAlpha             = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOpAlpha               = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].RenderTargetWriteMask      = 0;
    blendDisable.RenderTarget[0].RenderTargetWriteMask      = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStateReadWrite;
    depthStateReadWrite.DepthEnable                         = TRUE;
    depthStateReadWrite.DepthWriteMask                      = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStateReadWrite.DepthFunc                           = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    depthStateReadWrite.StencilEnable                       = FALSE;
    depthStateReadWrite.StencilReadMask                     = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateReadWrite.StencilWriteMask                    = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateReadWrite.FrontFace.StencilFunc               = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateReadWrite.FrontFace.StencilPassOp             = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilFailOp             = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilDepthFailOp        = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.BackFace                            = depthStateReadWrite.FrontFace;

    D3D12_SAMPLER_DESC defaultSamplerDesc;
    defaultSamplerDesc.Filter                               = D3D12_FILTER_ANISOTROPIC;
    defaultSamplerDesc.AddressU                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.AddressV                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.AddressW                             = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    defaultSamplerDesc.MipLODBias                           = 0.0f;
    defaultSamplerDesc.MaxAnisotropy                        = 8;
    defaultSamplerDesc.ComparisonFunc                       = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    defaultSamplerDesc.BorderColor[0]                       = 1.0f;
    defaultSamplerDesc.BorderColor[1]                       = 1.0f;
    defaultSamplerDesc.BorderColor[2]                       = 1.0f;
    defaultSamplerDesc.BorderColor[3]                       = 1.0f;
    defaultSamplerDesc.MinLOD                               = 0.0f;
    defaultSamplerDesc.MaxLOD                               = D3D12_FLOAT32_MAX;

    // Root sig setup
    {
        RealtimeRootSignature.Reset(4, 2);
        RealtimeRootSignature[0].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
        RealtimeRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        RealtimeRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
        RealtimeRootSignature[3].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.InitStaticSampler(0, defaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.InitStaticSampler(1, defaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.Finalize("FullscreenQuadRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // PSO setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        RealtimePSO.SetRootSignature(FullscreenQuadRootSignature);
        RealtimePSO.SetRasterizerState(rasterizerDesc);
        RealtimePSO.SetBlendState(blendDisable);
        RealtimePSO.SetDepthStencilState(depthStateReadWrite);
        RealtimePSO.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        RealtimePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimePSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RealtimePSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimePSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimePSO.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::LoadScene()
{
    // Clean up previous scene
    if (TheWorldScene != nullptr)
    {
        delete TheWorldScene;
        TheWorldScene = nullptr;
    }

    for (int i = 0; i < TheRealtimeSceneList.size(); i++)
    {
        delete TheRealtimeSceneList[i];
    }
    TheRealtimeSceneList.clear();

    // Load the selected scene
    TheWorldScene = GetSampleScene(SampleScene(sSampleScene));
    TheWorldScene->GetCamera().SetAspect((float)RenderDevice::Get().GetBackbufferWidth() / (float)RenderDevice::Get().GetBackbufferHeight());
    TheWorldScene->GetCamera().SetFocusDistanceToLookAt();
    sVertFov = TheWorldScene->GetCamera().GetVertFov();

    std::vector<DirectX::XMMATRIX>  matrixStack;
    std::vector<bool>               flipNormalStack;
    std::vector<SpotLight>          spotLightsList;
    GenerateRenderListFromWorld(TheWorldScene->GetWorld(), nullptr, TheRealtimeSceneList, spotLightsList, matrixStack, flipNormalStack);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::Raytrace(bool enable)
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

        RenderMode = RenderingMode_Cpu;
        TheRaytracer->BeginRaytrace(TheWorldScene, OnRaytraceComplete);
    }
    else
    {
        RenderMode = RenderingMode_Realtime;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRaytracer()
{
    int backbufferWidth  = RenderDevice::Get().GetBackbufferWidth();
    int backbufferHeight = RenderDevice::Get().GetBackbufferHeight();

    if (TheRaytracer != nullptr)
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    CPURaytracerTex.Destroy();
    CPURaytracerTex.Create("CpuRaytracerTex", backbufferWidth, backbufferHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

    // Create the ray tracer
    TheRaytracer = new Raytracer(backbufferWidth, backbufferHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads, true);

    // Reset the aspect ratio on the scene camera
    if (TheWorldScene != nullptr)
    {
        TheWorldScene->GetCamera().SetAspect((float)backbufferWidth / (float)backbufferHeight);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished)
{
    if (actuallyFinished)
    {
        WriteImageAndLog(tracer, "RaytracerWindows");
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnKeyDown(UINT8 key)
{
    switch (key)
    {
        case VK_SPACE:
        {
            Raytrace(true);
            break;
        }

        case VK_ESCAPE:
        {
            Raytrace(false);
            PostQuitMessage(0);
        }
        break;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnUpdate()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("Renderer::OnRender()");

    if (RenderMode == RenderingMode_Cpu)
    {
        renderContext.SetRootSignature(FullscreenQuadRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());
        renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        renderContext.ClearDepth(RenderDevice::Get().GetDepthStencil());
        renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());

        if (TheRaytracer != nullptr)
        {
            // Update GPU texture with the cpu traced results
            D3D12_SUBRESOURCE_DATA subresource;
            subresource.RowPitch   = 4 * TheRaytracer->GetOutputWidth();
            subresource.SlicePitch = subresource.RowPitch * TheRaytracer->GetOutputHeight();
            subresource.pData      = TheRaytracer->GetOutputBufferRGBA();
            renderContext.InitializeTexture(CPURaytracerTex, 1, &subresource);

            renderContext.TransitionResource(CPURaytracerTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.SetPipelineState(FullscreenPipelineState);
            renderContext.SetDynamicDescriptor(FullscreenQuadRootIndex_Texture0, 0, CPURaytracerTex.GetSRV());
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.SetNullVertexBuffer(0);
            renderContext.SetNullIndexBuffer();
            renderContext.Draw(3);
        }
    }


    renderContext.Finish();

    // Present
    RenderDevice::Get().Present();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnSizeChanged(uint32_t width, uint32_t height, bool minimized)
{
    if (!RenderDevice::Get().WindowSizeChanged(width, height, minimized))
    {
        return;
    }

    OnResizeRaytracer();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
}
