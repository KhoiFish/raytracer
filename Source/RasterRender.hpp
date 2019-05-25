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

#include "RaytracingShaderInclude.h"
#include <chrono>
#include <random>

// ----------------------------------------------------------------------------------------------------------------------------

static std::uniform_real_distribution<float>    sRngDist;
static std::mt19937                             sRng;

// ----------------------------------------------------------------------------------------------------------------------------

enum RealtimeRenderingRootIndex
{
    RealtimeRenderingRootIndex_ConstantBuffer0 = 0,
    RealtimeRenderingRootIndex_ConstantBuffer1,
    RealtimeRenderingRootIndex_Texture0,
    RealtimeRenderingRootIndex_Texture1,
    RealtimeRenderingRootIndex_Texture2,
    RealtimeRenderingRootIndex_Texture3,
    RealtimeRenderingRootIndex_Texture4,
    RealtimeRenderingRootIndex_Texture5,
    RealtimeRenderingRootIndex_Texture6,
    RealtimeRenderingRootIndex_Texture7,

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
    RealtimeRenderingRegisters_Texture5         = 5,
    RealtimeRenderingRegisters_Texture6         = 6,
    RealtimeRenderingRegisters_Texture7         = 7,
    RealtimeRenderingRegisters_LinearSampler    = 0,
    RealtimeRenderingRegisters_AnisoSampler     = 1,
};

// ----------------------------------------------------------------------------------------------------------------------------

const char* Renderer::GetSelectedBufferName()
{
    static const char* bufferNames[] =
    {
        "Composite Output",
        "Direct Lighting",
        "Indirect Lighting",
        "Ambient Occlusion",
        "Diffuse",
        "Cpu Raytrace",
    };

    if (SelectedBufferIndex >= ARRAY_SIZE(bufferNames))
    {
        return "Unknown";
    }
    else
    {
        return bufferNames[SelectedBufferIndex];
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupRasterRender()
{
    if (RasterDescriptorHeap != nullptr)
    {
        delete RasterDescriptorHeap;
        RasterDescriptorHeap = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRasterRender()
{
    // Delete old one, if it exists
    if (RasterDescriptorHeap != nullptr)
    {
        delete RasterDescriptorHeap;
        RasterDescriptorHeap = nullptr;
    }

    // Create heap for descriptors
    RasterDescriptorHeap = new DescriptorHeapStack(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);

    RasterDescriptorHeap->AllocateTexture2DSrv(
        DirectLightingAOBuffer[1].GetResource(),
        RaytracingBufferType,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    RasterDescriptorHeap->AllocateTexture2DSrv(
        IndirectLightingBuffer[1].GetResource(),
        RaytracingBufferType,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    RasterDescriptorHeap->AllocateTexture2DSrv(
        CPURaytracerTex.GetResource(),
        CPURaytracerTexType,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    for (int i = 0; i < DeferredBufferType_Num; i++)
    {
        RasterDescriptorHeap->AllocateTexture2DSrv(
            DeferredBuffers[i].GetResource(),
            DeferredBuffersRTTypes[i],
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_SAMPLER_DESC getLinearSamplerDesc()
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

    return linearSamplerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_SAMPLER_DESC getAnisoSamplerDesc()
{
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

    return anisoSamplerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_RASTERIZER_DESC getDefaultRasterState(bool frontCounterClockwise = false)
{
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode                 = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode                 = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise    = frontCounterClockwise ? TRUE : FALSE;
    rasterizerDesc.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable          = TRUE;
    rasterizerDesc.MultisampleEnable        = FALSE;
    rasterizerDesc.AntialiasedLineEnable    = FALSE;
    rasterizerDesc.ForcedSampleCount        = 0;
    rasterizerDesc.ConservativeRaster       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    return rasterizerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_BLEND_DESC getBlendDisabledState()
{
    D3D12_BLEND_DESC blendDisable = {};
    blendDisable.IndependentBlendEnable                 = FALSE;
    blendDisable.RenderTarget[0].BlendEnable            = FALSE;
    blendDisable.RenderTarget[0].SrcBlend               = D3D12_BLEND_SRC_ALPHA;
    blendDisable.RenderTarget[0].DestBlend              = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOp                = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].SrcBlendAlpha          = D3D12_BLEND_ONE;
    blendDisable.RenderTarget[0].DestBlendAlpha         = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOpAlpha           = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].RenderTargetWriteMask  = 0;
    blendDisable.RenderTarget[0].RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL;

    return blendDisable;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_DEPTH_STENCIL_DESC getDepthEnabledState()
{
    D3D12_DEPTH_STENCIL_DESC depthStateReadWrite;
    depthStateReadWrite.DepthEnable                     = TRUE;
    depthStateReadWrite.DepthWriteMask                  = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStateReadWrite.DepthFunc                       = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStateReadWrite.StencilEnable                   = FALSE;
    depthStateReadWrite.StencilReadMask                 = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateReadWrite.StencilWriteMask                = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateReadWrite.FrontFace.StencilFunc           = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateReadWrite.FrontFace.StencilPassOp         = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilFailOp         = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilDepthFailOp    = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.BackFace                        = depthStateReadWrite.FrontFace;

    return depthStateReadWrite;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline D3D12_DEPTH_STENCIL_DESC getDepthDisabledState()
{
    D3D12_DEPTH_STENCIL_DESC depthStateDisabled = getDepthEnabledState();
    depthStateDisabled.DepthEnable     = FALSE;
    depthStateDisabled.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ZERO;

    return depthStateDisabled;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRasterPipeline()
{
    CpuResultsBufferIndex = RealtimeRenderingRegisters_Texture4 + 1;   

    // Init random generator
    {
        auto currentTime    = std::chrono::high_resolution_clock::now();
        auto timeInMillisec = std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime);
        sRng                = std::mt19937(uint32_t(timeInMillisec.time_since_epoch().count()));
    }

    // Root sig setup
    {
        const int numSamplers = 2;
        RasterRootSignature.Reset(RealtimeRenderingRootIndex_Num, numSamplers);
        RasterRootSignature[RealtimeRenderingRootIndex_ConstantBuffer0].InitAsConstantBuffer(RealtimeRenderingRegisters_ConstantBuffer0, D3D12_SHADER_VISIBILITY_ALL);
        RasterRootSignature[RealtimeRenderingRootIndex_ConstantBuffer1].InitAsConstantBuffer(RealtimeRenderingRegisters_ConstantBuffer1, D3D12_SHADER_VISIBILITY_ALL);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture0, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture1, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture2, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture3, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture4, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture5, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture6].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture6, 1, 0);
        RasterRootSignature[RealtimeRenderingRootIndex_Texture7].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture7, 1, 0);
        RasterRootSignature.InitStaticSampler(RealtimeRenderingRegisters_LinearSampler, getLinearSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
        RasterRootSignature.InitStaticSampler(RealtimeRenderingRegisters_AnisoSampler, getAnisoSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
        RasterRootSignature.Finalize("RealtimeRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // This will create descriptors
        OnResizeRasterRender();
    }

    // Geometry pass PSO setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_PS.cso", &pixelShaderBlob));

        RasterGeometryPassPSO.SetRootSignature(RasterRootSignature);
        RasterGeometryPassPSO.SetRasterizerState(getDefaultRasterState());
        RasterGeometryPassPSO.SetBlendState(getBlendDisabledState());
        RasterGeometryPassPSO.SetDepthStencilState(getDepthEnabledState());
        RasterGeometryPassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
        RasterGeometryPassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RasterGeometryPassPSO.SetRenderTargetFormats(_countof(DeferredBuffersRTTypes), DeferredBuffersRTTypes, RenderDevice::Get().GetDepthBufferFormat());
        RasterGeometryPassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RasterGeometryPassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RasterGeometryPassPSO.Finalize();
    }

    // Composite pass PSO setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        RasterCompositePassPSO.SetRootSignature(RasterRootSignature);
        RasterCompositePassPSO.SetRasterizerState(getDefaultRasterState(true));
        RasterCompositePassPSO.SetBlendState(getBlendDisabledState());
        RasterCompositePassPSO.SetDepthStencilState(getDepthDisabledState());
        RasterCompositePassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
        RasterCompositePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RasterCompositePassPSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RasterCompositePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RasterCompositePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RasterCompositePassPSO.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGeometryPass()
{
    // Jitter camera
    {
        if (UserInput.GpuCameraJitter)
        {
            // Half pixel offset
            float xOff = (float(sRngDist(sRng)) - 0.5f);
            float yOff = (float(sRngDist(sRng)) - 0.5f);
            TheRenderScene->GetCamera().SetJitter(xOff / float(Width), yOff / float(Height));
        }
        else
        {
            TheRenderScene->GetCamera().SetJitter(0, 0);
        }
    }

    GraphicsContext& renderContext = GraphicsContext::Begin("RenderRealtimeResults");
    {
        renderContext.SetRootSignature(RasterRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());

        // Geometry pass
        {
            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.TransitionResource(DeferredBuffers[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
            
            // Bind gbuffer
            D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]
            {
                DeferredBuffers[DeferredBufferType_Position].GetRTV(),
                DeferredBuffers[DeferredBufferType_Normal].GetRTV(),
                DeferredBuffers[DeferredBufferType_TexCoordAndDepth].GetRTV(),
                DeferredBuffers[DeferredBufferType_Albedo].GetRTV()
            };
            renderContext.SetRenderTargets(ARRAYSIZE(rtvs), rtvs, RenderDevice::Get().GetDepthStencil().GetDSV());

            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.ClearColor(DeferredBuffers[i]);
            }

            renderContext.ClearDepth(RenderDevice::Get().GetDepthStencil());
            renderContext.SetPipelineState(RasterGeometryPassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.FlushResourceBarriers();
            RenderSceneList(renderContext);
        }
    }
    renderContext.Finish();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderCompositePass()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderCompositePass");
    {
        renderContext.SetRootSignature(RasterRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());

        // Composite pass
        {
            // Update GPU texture with the cpu traced results
            if (TheRaytracer != nullptr)
            {
                D3D12_SUBRESOURCE_DATA subresource;
                subresource.RowPitch    = 4 * TheRaytracer->GetOutputWidth();
                subresource.SlicePitch  = subresource.RowPitch * TheRaytracer->GetOutputHeight();
                subresource.pData       =  TheRaytracer->GetOutputBufferRGBA();
                renderContext.InitializeTexture(CPURaytracerTex, 1, &subresource);
            }

            // Setup constants
            SceneConstantBuffer sceneCb;
            {
                // The following multipliers let us select which buffer to display
                // See the CompositePass shader for more details.
                const XMFLOAT4 bufferOn  = XMFLOAT4(1, 1, 1, 1);
                const XMFLOAT4 bufferOff = XMFLOAT4(0, 0, 0, 0);
                for (int i = 0; i < ARRAY_SIZE(sceneCb.TextureMultipliers); i++)
                {
                    if (SelectedBufferIndex == i || SelectedBufferIndex == 0)
                    {
                        sceneCb.TextureMultipliers[i] = bufferOn;
                    }
                    else
                    {
                        sceneCb.TextureMultipliers[i] = bufferOff;
                    }
                }

                if (SelectedBufferIndex == 0)
                {
                    sceneCb.CompositeMultipliers[0] = bufferOn;
                    sceneCb.CompositeMultipliers[1] = bufferOff;
                }
                else
                {
                    sceneCb.CompositeMultipliers[0] = bufferOff;
                    sceneCb.CompositeMultipliers[1] = bufferOn;
                }

                // Setup direct/indirect multipliers
                sceneCb.DirectIndirectLightMult = XMFLOAT2(UserInput.GpuDirectLightMult, UserInput.GpuIndirectLightMult);
            }

            // Transition resources
            renderContext.TransitionResource(CPURaytracerTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(DirectLightingAOBuffer[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(IndirectLightingBuffer[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.TransitionResource(DeferredBuffers[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ);

            // Set descriptor heaps and tables
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RasterDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RasterDescriptorHeap->GetCount(); i++)
            {
                uint32_t rootIndex = RealtimeRenderingRootIndex_Texture0 + i;
                renderContext.SetDescriptorTable(rootIndex, RasterDescriptorHeap->GetGpuHandle(i));
            }

            // Setup rest of the pipeline
            renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(sceneCb), &sceneCb);
            renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
            renderContext.SetPipelineState(RasterCompositePassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.FlushResourceBarriers();

            // Draw
            renderContext.SetNullVertexBuffer(0);
            renderContext.SetNullIndexBuffer();
            renderContext.Draw(3);
        }
    }
    renderContext.Finish();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupSceneConstantBuffer(const FXMMATRIX& model, SceneConstantBuffer& sceneCB)
{
    RealtimeCamera& camera = TheRenderScene->GetCamera();

    sceneCB.FarClipDist = camera.GetZFar();

    XMStoreFloat4(&sceneCB.CameraPosition, camera.GetEye());
    XMStoreFloat4x4(&sceneCB.ModelMatrix, XMMatrixTranspose(model));
    XMStoreFloat4x4(&sceneCB.ViewMatrix, XMMatrixTranspose(camera.GetViewMatrix()));
    XMStoreFloat4x4(&sceneCB.ProjectionMatrix, XMMatrixTranspose(camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.ModelViewMatrix, XMMatrixTranspose(model * camera.GetViewMatrix()));
    XMStoreFloat4x4(&sceneCB.ViewProjectionMatrix, XMMatrixTranspose(camera.GetViewMatrix() * camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.ModelViewProjectionMatrix, XMMatrixTranspose(model * camera.GetViewMatrix() * camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.InverseTransposeModelViewMatrix, XMMatrixInverse(nullptr, model * camera.GetViewMatrix()));
    XMStoreFloat4x4(&sceneCB.InverseViewProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix())));
    XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix()));
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderSceneList(GraphicsContext& renderContext)
{
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        SceneConstantBuffer matrices;
        SetupSceneConstantBuffer(TheRenderScene->GetRenderSceneList()[i]->WorldMatrix, matrices);

        RealtimeEngine::Texture* defaultTexture = &RealtimeEngine::TextureManager::GetWhiteTex2D();
        RealtimeEngine::Texture* diffuseTex     = (TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture != nullptr) ? TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture : defaultTexture;

        renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(matrices), &matrices);
        renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer1, sizeof(TheRenderScene->GetRenderSceneList()[i]->Material), &TheRenderScene->GetRenderSceneList()[i]->Material);

        renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RenderDevice::Get().GetDefaultDescriptorHeapStack().GetDescriptorHeap());
        renderContext.SetDescriptorTable(RealtimeRenderingRootIndex_Texture0, diffuseTex->GetGpuHandle());

        renderContext.SetVertexBuffer(0, TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.IndexBufferView());
        renderContext.DrawIndexed((uint32_t)TheRenderScene->GetRenderSceneList()[i]->Indices.size());
    }
}
