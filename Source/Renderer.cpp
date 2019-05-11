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

#define DEFAULT_TEXTURE_FILENAME    "RuntimeData/guitar.jpg"

// ----------------------------------------------------------------------------------------------------------------------------

static int     sNumSamplesPerRay = 500;
static int     sMaxScatterDepth  = 50;
static int     sNumThreads       = 4;
static float   sVertFov          = 40.f;
static int     sSampleScene      = SceneMesh;
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

enum RealtimeRenderingRootIndex
{
    RealtimeRenderingRootIndex_ConstantBuffer0 = 0,
    RealtimeRenderingRootIndex_ConstantBuffer1,
    RealtimeRenderingRootIndex_Texture0,
    RealtimeRenderingRootIndex_Texture1,
    RealtimeRenderingRootIndex_Texture2,
    RealtimeRenderingRootIndex_Texture3,
    RealtimeRenderingRootIndex_Texture4,

    RealtimeRenderingRootIndex_Num
};

enum RealtimeRenderingRegisters
{
    RealtimeRenderingRegisters_ConstantBuffer0  = 0,
    RealtimeRenderingRegisters_ConstantBuffer1  = 1,
    RealtimeRenderingRegisters_Texture0         = 0,
    RealtimeRenderingRegisters_Texture1         = 1,
    RealtimeRenderingRegisters_Texture2         = 2,
    RealtimeRenderingRegisters_Texture3         = 3,
    RealtimeRenderingRegisters_Texture4         = 4,
    RealtimeRenderingRegisters_LinearSampler    = 0,
    RealtimeRenderingRegisters_AnisoSampler     = 1,
};

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface(width, height)
    , RenderMode(RenderingMode_Realtime)
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
    OnResizeRaytracer();
    SetupRenderBuffers();
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
    SetupRenderBuffers();

    // Load the scene data
    LoadScene();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupFullscreenQuadPipeline()
{
    // Fullscreen quad signature setup
    {
        D3D12_SAMPLER_DESC defaultSamplerDesc;
        defaultSamplerDesc.Filter           = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        defaultSamplerDesc.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        defaultSamplerDesc.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        defaultSamplerDesc.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        defaultSamplerDesc.MipLODBias       = 0.0f;
        defaultSamplerDesc.MaxAnisotropy    = 8;
        defaultSamplerDesc.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        defaultSamplerDesc.BorderColor[0]   = 1.0f;
        defaultSamplerDesc.BorderColor[1]   = 1.0f;
        defaultSamplerDesc.BorderColor[2]   = 1.0f;
        defaultSamplerDesc.BorderColor[3]   = 1.0f;
        defaultSamplerDesc.MinLOD           = 0.0f;
        defaultSamplerDesc.MaxLOD           = D3D12_FLOAT32_MAX;

        const int numSamplers = 1;
        FullscreenQuadRootSignature.Reset(FullscreenQuadRootIndex_Num, numSamplers);
        FullscreenQuadRootSignature[FullscreenQuadRootIndex_Texture0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, FullscreenQuadRegisters_Texture0, 1);
        FullscreenQuadRootSignature.InitStaticSampler(FullscreenQuadRegisters_LinearSampler, defaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        FullscreenQuadRootSignature.Finalize("FullscreenQuadRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // Full screen pipeline state setup
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

        D3D12_DEPTH_STENCIL_DESC depthDisabledState;
        depthDisabledState.DepthEnable                          = FALSE;
        depthDisabledState.DepthWriteMask                       = D3D12_DEPTH_WRITE_MASK_ALL;
        depthDisabledState.DepthFunc                            = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        depthDisabledState.StencilEnable                        = FALSE;
        depthDisabledState.StencilReadMask                      = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthDisabledState.StencilWriteMask                     = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        depthDisabledState.FrontFace.StencilFunc                = D3D12_COMPARISON_FUNC_ALWAYS;
        depthDisabledState.FrontFace.StencilPassOp              = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.FrontFace.StencilFailOp              = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.FrontFace.StencilDepthFailOp         = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.BackFace                             = depthDisabledState.FrontFace;

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        FullscreenPipelineState.SetRootSignature(FullscreenQuadRootSignature);
        FullscreenPipelineState.SetRasterizerState(rasterizerDesc);
        FullscreenPipelineState.SetBlendState(blendDisable);
        FullscreenPipelineState.SetDepthStencilState(depthDisabledState);
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
    // Root sig setup
    {
        D3D12_SAMPLER_DESC linearSamplerDesc;
        linearSamplerDesc.Filter            = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        linearSamplerDesc.AddressU          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSamplerDesc.AddressV          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSamplerDesc.AddressW          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSamplerDesc.MipLODBias        = 0.0f;
        linearSamplerDesc.MaxAnisotropy     = 8;
        linearSamplerDesc.ComparisonFunc    = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        linearSamplerDesc.BorderColor[0]    = 1.0f;
        linearSamplerDesc.BorderColor[1]    = 1.0f;
        linearSamplerDesc.BorderColor[2]    = 1.0f;
        linearSamplerDesc.BorderColor[3]    = 1.0f;
        linearSamplerDesc.MinLOD            = 0.0f;
        linearSamplerDesc.MaxLOD            = D3D12_FLOAT32_MAX;

        D3D12_SAMPLER_DESC anisoSamplerDesc;
        anisoSamplerDesc.Filter             = D3D12_FILTER_ANISOTROPIC;
        anisoSamplerDesc.AddressU           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        anisoSamplerDesc.AddressV           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        anisoSamplerDesc.AddressW           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        anisoSamplerDesc.MipLODBias         = 0.0f;
        anisoSamplerDesc.MaxAnisotropy      = 8;
        anisoSamplerDesc.ComparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        anisoSamplerDesc.BorderColor[0]     = 1.0f;
        anisoSamplerDesc.BorderColor[1]     = 1.0f;
        anisoSamplerDesc.BorderColor[2]     = 1.0f;
        anisoSamplerDesc.BorderColor[3]     = 1.0f;
        anisoSamplerDesc.MinLOD             = 0.0f;
        anisoSamplerDesc.MaxLOD             = D3D12_FLOAT32_MAX;

        const int numSamplers = 2;
        RealtimeRootSignature.Reset(RealtimeRenderingRootIndex_Num, numSamplers);
        RealtimeRootSignature[RealtimeRenderingRootIndex_ConstantBuffer0].InitAsConstantBuffer(RealtimeRenderingRegisters_ConstantBuffer0, D3D12_SHADER_VISIBILITY_ALL);
        RealtimeRootSignature[RealtimeRenderingRootIndex_ConstantBuffer1].InitAsConstantBuffer(RealtimeRenderingRegisters_ConstantBuffer1, D3D12_SHADER_VISIBILITY_ALL);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture0, 1);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture1, 1);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture2, 1);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture3, 1);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture4, 1);
        RealtimeRootSignature.InitStaticSampler(RealtimeRenderingRegisters_LinearSampler, linearSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.InitStaticSampler(RealtimeRenderingRegisters_AnisoSampler, anisoSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.Finalize("RealtimeRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // Z pre pass PSO setup
    {
        D3D12_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.FillMode                                 = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode                                 = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise                    = FALSE;
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
        depthStateReadWrite.DepthFunc                           = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStateReadWrite.StencilEnable                       = FALSE;
        depthStateReadWrite.StencilReadMask                     = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthStateReadWrite.StencilWriteMask                    = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        depthStateReadWrite.FrontFace.StencilFunc               = D3D12_COMPARISON_FUNC_ALWAYS;
        depthStateReadWrite.FrontFace.StencilPassOp             = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.FrontFace.StencilFailOp             = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.FrontFace.StencilDepthFailOp        = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.BackFace                            = depthStateReadWrite.FrontFace;

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\ZPrePass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\ZPrePass_PS.cso", &pixelShaderBlob));

        ZBufferRTType = DXGI_FORMAT_R32_FLOAT;

        RealtimeZPrePassPSO.SetRootSignature(RealtimeRootSignature);
        RealtimeZPrePassPSO.SetRasterizerState(rasterizerDesc);
        RealtimeZPrePassPSO.SetBlendState(blendDisable);
        RealtimeZPrePassPSO.SetDepthStencilState(depthStateReadWrite);
        RealtimeZPrePassPSO.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        RealtimeZPrePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimeZPrePassPSO.SetRenderTargetFormats(1, &ZBufferRTType, RenderDevice::Get().GetDepthBufferFormat());
        RealtimeZPrePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimeZPrePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimeZPrePassPSO.Finalize();
    }

    // Geometry pass PSO setup
    {
        D3D12_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.FillMode                                 = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode                                 = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise                    = FALSE;
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
        depthStateReadWrite.DepthFunc                           = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStateReadWrite.StencilEnable                       = FALSE;
        depthStateReadWrite.StencilReadMask                     = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthStateReadWrite.StencilWriteMask                    = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        depthStateReadWrite.FrontFace.StencilFunc               = D3D12_COMPARISON_FUNC_ALWAYS;
        depthStateReadWrite.FrontFace.StencilPassOp             = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.FrontFace.StencilFailOp             = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.FrontFace.StencilDepthFailOp        = D3D12_STENCIL_OP_KEEP;
        depthStateReadWrite.BackFace                            = depthStateReadWrite.FrontFace;

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_PS.cso", &pixelShaderBlob));

        DeferredBuffersRTTypes[DeferredBufferType_Position] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        DeferredBuffersRTTypes[DeferredBufferType_Normal]   = DXGI_FORMAT_R32G32B32A32_FLOAT;
        DeferredBuffersRTTypes[DeferredBufferType_TexCoord] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        DeferredBuffersRTTypes[DeferredBufferType_Diffuse]  = DXGI_FORMAT_R32G32B32A32_FLOAT;

        RealtimeGeometryPassPSO.SetRootSignature(RealtimeRootSignature);
        RealtimeGeometryPassPSO.SetRasterizerState(rasterizerDesc);
        RealtimeGeometryPassPSO.SetBlendState(blendDisable);
        RealtimeGeometryPassPSO.SetDepthStencilState(depthStateReadWrite);
        RealtimeGeometryPassPSO.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        RealtimeGeometryPassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimeGeometryPassPSO.SetRenderTargetFormats(_countof(DeferredBuffersRTTypes), DeferredBuffersRTTypes, RenderDevice::Get().GetDepthBufferFormat());
        RealtimeGeometryPassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimeGeometryPassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimeGeometryPassPSO.Finalize();
    }

    // Composite pass PSO setup
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

        D3D12_DEPTH_STENCIL_DESC depthDisabledState;
        depthDisabledState.DepthEnable                          = FALSE;
        depthDisabledState.DepthWriteMask                       = D3D12_DEPTH_WRITE_MASK_ALL;
        depthDisabledState.DepthFunc                            = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        depthDisabledState.StencilEnable                        = FALSE;
        depthDisabledState.StencilReadMask                      = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthDisabledState.StencilWriteMask                     = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        depthDisabledState.FrontFace.StencilFunc                = D3D12_COMPARISON_FUNC_ALWAYS;
        depthDisabledState.FrontFace.StencilPassOp              = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.FrontFace.StencilFailOp              = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.FrontFace.StencilDepthFailOp         = D3D12_STENCIL_OP_KEEP;
        depthDisabledState.BackFace                             = depthDisabledState.FrontFace;

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        RealtimeCompositePassPSO.SetRootSignature(RealtimeRootSignature);
        RealtimeCompositePassPSO.SetRasterizerState(rasterizerDesc);
        RealtimeCompositePassPSO.SetBlendState(blendDisable);
        RealtimeCompositePassPSO.SetDepthStencilState(depthDisabledState);
        RealtimeCompositePassPSO.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        RealtimeCompositePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimeCompositePassPSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RealtimeCompositePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRenderBuffers()
{
    uint32_t width = RenderDevice::Get().GetBackbufferWidth();
    uint32_t height = RenderDevice::Get().GetBackbufferHeight();

    for (int i = 0; i < DeferredBufferType_Num; i++)
    {
        DeferredBuffers[i].Destroy();
        DeferredBuffers[i].Create(DeferredBufferTypeStrings[i], width, height, 1, DeferredBuffersRTTypes[i]);
    }

    ZPrePassBuffer.Destroy();
    ZPrePassBuffer.Create("ZPrePass Buffer", width, height, 1, ZBufferRTType);
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

    // Load default texture
    RealtimeEngine::TextureManager::LoadFromFile(DEFAULT_TEXTURE_FILENAME);
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

void Renderer::OnSizeChanged(uint32_t width, uint32_t height, bool minimized)
{
    if (!RenderDevice::Get().WindowSizeChanged(width, height, minimized))
    {
        return;
    }

    OnResizeRaytracer();
    SetupRenderBuffers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
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
            break;
        }

        case VK_SHIFT:
        {
            UserInput.ShiftKeyPressed = true;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            UserInput.Forward = 1.0f;
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            UserInput.Backward = 1.0f;
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            UserInput.Left = 1.0f;
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            UserInput.Right = 1.0f;
            break;
        }

        case 'Q':
        case 'q':
        {
            UserInput.Down = 1.0f;
            break;
        }
        

        case 'E':
        case 'e':
        {
            UserInput.Up = 1.0f;
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnKeyUp(uint8_t key)
{
    switch (key)
    {
        case VK_SHIFT:
        {
            UserInput.ShiftKeyPressed = false;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            UserInput.Forward = 0.0f;
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            UserInput.Backward = 0.0f;
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            UserInput.Left = 0.0f;
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            UserInput.Right = 0.0f;
            break;
        }

        case 'Q':
        case 'q':
        {
            UserInput.Down = 0.0f;
            break;
        }


        case 'E':
        case 'e':
        {
            UserInput.Up = 0.0f;
            break;
        }

        case 'R':
        case 'r':
        {
            RenderMode = (RenderingMode)(((int)RenderMode + 1) % (int)MaxRenderModes);
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnMouseMove(uint32_t x, uint32_t y)
{
    if (UserInput.LeftMouseButtonPressed)
    {
        UserInput.MouseDx    = int(x) - UserInput.PrevMouseX;
        UserInput.MouseDy    = int(y) - UserInput.PrevMouseY;
        UserInput.PrevMouseX = x;
        UserInput.PrevMouseY = y;

        Raytrace(false);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonDown(uint32_t x, uint32_t y)
{
    UserInput.LeftMouseButtonPressed = true;
    UserInput.PrevMouseX             = x;
    UserInput.PrevMouseY             = y;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonUp(uint32_t x, uint32_t y)
{
    UserInput.LeftMouseButtonPressed = false;
    UserInput.PrevMouseX             = x;
    UserInput.PrevMouseY             = y;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnUpdate()
{
    float speedMultipler = (UserInput.ShiftKeyPressed ? 64.f : 32.0f) * 16.f;
    float scale          = speedMultipler * 0.001f; // *float(e.ElapsedTime);
    float forwardAmount  = (UserInput.Forward - UserInput.Backward) * scale;
    float strafeAmount   = (UserInput.Left    - UserInput.Right)    * scale;
    float upDownAmount   = (UserInput.Up      - UserInput.Down)     * scale;

    UpdateCameras(sVertFov, forwardAmount, strafeAmount, upDownAmount, UserInput.MouseDx, UserInput.MouseDy, TheWorldScene->GetCamera(), TheRenderCamera);
    UserInput.MouseDx = 0;
    UserInput.MouseDy = 0;
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

void Renderer::OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished)
{
    if (actuallyFinished)
    {
        WriteImageAndLog(tracer, "RaytracerWindows");
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void XM_CALLCONV ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX projection, RenderMatrices& mat)
{
    XMStoreFloat4x4(&mat.ModelMatrix, XMMatrixTranspose(model));
    XMStoreFloat4x4(&mat.ViewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mat.ProjectionMatrix, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&mat.ModelViewMatrix, XMMatrixTranspose(model * view));
    XMStoreFloat4x4(&mat.InverseTransposeModelViewMatrix, XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(nullptr, model * view))));
    XMStoreFloat4x4(&mat.ModelViewProjectionMatrix, XMMatrixTranspose(model * view * projection));
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderCPUResults()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderCPUResults");
    {
        renderContext.SetRootSignature(FullscreenQuadRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());
        renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        renderContext.ClearDepth(RenderDevice::Get().GetDepthStencil());
        renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
        renderContext.ClearColor(RenderDevice::Get().GetRenderTarget());

        if (TheRaytracer != nullptr)
        {
            // Update GPU texture with the cpu traced results
            D3D12_SUBRESOURCE_DATA subresource;
            subresource.RowPitch = 4 * TheRaytracer->GetOutputWidth();
            subresource.SlicePitch = subresource.RowPitch * TheRaytracer->GetOutputHeight();
            subresource.pData = TheRaytracer->GetOutputBufferRGBA();
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
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderRealtimeResults()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderRealtimeResults");
    {
        renderContext.SetRootSignature(RealtimeRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());
        RenderDevice::Get().GetDepthStencil().SetClearDepthValue(1.0f);

        // Z pre pass
        {
            renderContext.TransitionResource(ZPrePassBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
            renderContext.ClearDepth(RenderDevice::Get().GetDepthStencil());
            renderContext.SetPipelineState(RealtimeZPrePassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.SetRenderTarget(ZPrePassBuffer.GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
            renderContext.ClearColor(ZPrePassBuffer);

            RenderSceneList(renderContext, TheRenderCamera.get_ViewMatrix(), TheRenderCamera.get_ProjectionMatrix());
        }

        // Geometry pass
        {
            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.TransitionResource(DeferredBuffers[i], D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ, true);
            renderContext.SetPipelineState(RealtimeGeometryPassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Bind gbuffer
            D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]
            {
                DeferredBuffers[DeferredBufferType_Position].GetRTV(),
                DeferredBuffers[DeferredBufferType_Normal].GetRTV(),
                DeferredBuffers[DeferredBufferType_TexCoord].GetRTV(),
                DeferredBuffers[DeferredBufferType_Diffuse].GetRTV()
            };
            renderContext.SetRenderTargets(ARRAYSIZE(rtvs), rtvs, RenderDevice::Get().GetDepthStencil().GetDSV());

            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.ClearColor(DeferredBuffers[i]);
            }

            RenderSceneList(renderContext, TheRenderCamera.get_ViewMatrix(), TheRenderCamera.get_ProjectionMatrix());
        }

        // Composite pass
        {
            renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ, true);
            renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
            renderContext.SetPipelineState(RealtimeCompositePassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.SetDynamicDescriptor(RealtimeRenderingRootIndex_Texture0, 0, ZPrePassBuffer.GetSRV());

            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.TransitionResource(DeferredBuffers[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
                renderContext.SetDynamicDescriptor(RealtimeRenderingRootIndex_Texture1 + i, 0, DeferredBuffers[i].GetSRV());
            }

            renderContext.SetNullVertexBuffer(0);
            renderContext.SetNullIndexBuffer();
            renderContext.Draw(3);
        }
    }
    renderContext.Finish();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderSceneList(GraphicsContext& renderContext, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    for (int i = 0; i < TheRealtimeSceneList.size(); i++)
    {
        // Don't render light shapes
        if (TheRealtimeSceneList[i]->Hitable->IsALightShape())
        {
            continue;
        }

        RenderMatrices matrices;
        ComputeMatrices(TheRealtimeSceneList[i]->WorldMatrix, viewMatrix, projectionMatrix, matrices);

        const RealtimeEngine::Texture* defaultTexture = RealtimeEngine::TextureManager::LoadFromFile(DEFAULT_TEXTURE_FILENAME);
        const RealtimeEngine::Texture* diffuseTex     = (TheRealtimeSceneList[i]->DiffuseTexture != nullptr) ? TheRealtimeSceneList[i]->DiffuseTexture : defaultTexture;

        renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(matrices), &matrices);
        renderContext.SetDynamicDescriptor(RealtimeRenderingRootIndex_Texture0, 0, diffuseTex->GetSRV());
        renderContext.SetVertexBuffer(0, TheRealtimeSceneList[i]->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(TheRealtimeSceneList[i]->IndexBuffer.IndexBufferView());
        renderContext.DrawIndexed((uint32_t)TheRealtimeSceneList[i]->Indices.size());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    // Render the right stuff
    switch (RenderMode)
    {
    case RenderingMode_Cpu:
        RenderCPUResults();
        break;

    case RenderingMode_Realtime:
        RenderRealtimeResults();
        break;

    default:
        break;
    }

    // Present
    RenderDevice::Get().Present();
}
