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

// ----------------------------------------------------------------------------------------------------------------------------

namespace RasterRenderRootSig
{
    enum EnumTypes
    {
        SceneCB = 0,
        InstanceCB,
        MaterialCB,
        CompositeCB,

        DirectLightAOTex,
        IndirectLightTex,
        CpuResultsTex,
        PositionsTex,
        NormalsTex,
        TexCoordsTex,
        DiffuseTex,

        Num,
    };

    enum IndexTypes
    {
        Register = 0,
        Count,
        Space,
        RangeType,
    };

    // [register, count, space, D3D12_DESCRIPTOR_RANGE_TYPE]
    static UINT Range[RasterRenderRootSig::Num][4] =
    {
        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // SceneCB
        { 1, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // InstanceCB
        { 2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // MaterialCB
        { 3, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // CompositeCB

        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // DirectLightAOTex   
        { 1, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // IndirectLightTex   
        { 2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // CpuResultsTex   
        { 3, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PositionsTex
        { 4, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // NormalsTex  
        { 5, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // TexCoordsTex
        { 6, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // DiffuseTex  
    };
}

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
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRasterRender()
{
    SetupRasterDescriptors();
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
    CpuResultsBufferIndex = RasterRenderRootSig::CpuResultsTex;

    // Init random generator
    {
        auto currentTime    = std::chrono::high_resolution_clock::now();
        auto timeInMillisec = std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime);
        RandGen             = std::mt19937(uint32_t(timeInMillisec.time_since_epoch().count()));
    }

    // Constant buffers
    RasterSceneConstantBuffer.Create(L"Raster Scene Constant Buffer", 1, (uint32_t)AlignUp(sizeof(SceneConstantBuffer), 256));
    RasterCompositeConstantBuffer.Create(L"Raster Composite Constant Buffer", 1, (uint32_t)AlignUp(sizeof(CompositeConstantBuffer), 256));

    // Root sig setup
    {
        const int numSamplers = 2;
        RasterRootSignature.Reset(RasterRenderRootSig::Num, numSamplers);
        {
            for (int i = 0; i < RasterRenderRootSig::Num; i++)
            {
                RasterRootSignature[i].InitAsDescriptorRange(
                    (D3D12_DESCRIPTOR_RANGE_TYPE)RasterRenderRootSig::Range[i][RasterRenderRootSig::RangeType],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Register],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Count],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Space],
                    D3D12_SHADER_VISIBILITY_ALL);
            }

            RasterRootSignature.InitStaticSampler(0, getLinearSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
            RasterRootSignature.InitStaticSampler(1, getAnisoSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
        }
        RasterRootSignature.Finalize("RasterRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
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

    // Setup descriptors
    SetupRasterDescriptors();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRasterDescriptors()
{
    RasterDescriptorIndexStart = RendererDescriptorHeap->GetCount();

    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    // Hack, this a placeholder for instance CB
    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    // Hack, this a placeholder for material CB
    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    RendererDescriptorHeap->AllocateBufferCbv(
        RasterCompositeConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterCompositeConstantBuffer.GetBufferSize());

    RendererDescriptorHeap->AllocateTexture2DSrv(
        DirectLightingAOBuffer[1].GetResource(),
        RaytracingBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        IndirectLightingBuffer[1].GetResource(),
        RaytracingBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        CPURaytracerTex.GetResource(),
        CPURaytracerTexType);

    for (int i = 0; i < DeferredBufferType_Num; i++)
    {
        RendererDescriptorHeap->AllocateTexture2DSrv(
            DeferredBuffers[i].GetResource(),
            DeferredBuffersRTTypes[i]);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupSceneConstantBuffer(SceneConstantBuffer& sceneCB)
{
    RealtimeCamera& camera = TheRenderScene->GetCamera();

    sceneCB.FarClipDist = camera.GetZFar();

    XMStoreFloat4(&sceneCB.CameraPosition, camera.GetEye());
    XMStoreFloat4x4(&sceneCB.ViewMatrix, XMMatrixTranspose(camera.GetViewMatrix()));
    XMStoreFloat4x4(&sceneCB.ProjectionMatrix, XMMatrixTranspose(camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.ViewProjectionMatrix, XMMatrixTranspose(camera.GetViewMatrix() * camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.InverseViewProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix())));
    XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix()));
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderSceneList(GraphicsContext& renderContext)
{
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RealtimeSceneNode* pNode = TheRenderScene->GetRenderSceneList()[i];

        renderContext.SetDescriptorTable(RasterRenderRootSig::InstanceCB, RendererDescriptorHeap->GetGpuHandle(pNode->InstanceDataHeapIndex));
        renderContext.SetDescriptorTable(RasterRenderRootSig::MaterialCB, RendererDescriptorHeap->GetGpuHandle(pNode->MaterialHeapIndex));
        renderContext.SetDescriptorTable(RasterRenderRootSig::DiffuseTex, RendererDescriptorHeap->GetGpuHandle(pNode->DiffuseTextureHeapIndex));
        renderContext.SetVertexBuffer(0, TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.IndexBufferView());
        renderContext.FlushResourceBarriers();
        renderContext.DrawIndexed((uint32_t)TheRenderScene->GetRenderSceneList()[i]->Indices.size());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGeometryPass()
{
    // Jitter camera
    {
        if (TheUserInputData.GpuCameraJitter)
        {
            // Half pixel offset
            float xOff = (float(RandDist(RandGen)) - 0.5f);
            float yOff = (float(RandDist(RandGen)) - 0.5f);
            TheRenderScene->GetCamera().SetJitter(xOff / float(Width), yOff / float(Height));
        }
        else
        {
            TheRenderScene->GetCamera().SetJitter(0, 0);
        }
    }

    GraphicsContext& renderContext = GraphicsContext::Begin("Geometry Pass");
    {
        renderContext.SetRootSignature(RasterRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());

        // Geometry pass
        {
            // Update scene constant buffer
            SceneConstantBuffer sceneCB;
            SetupSceneConstantBuffer(sceneCB);
            RasterSceneConstantBuffer.Upload(&sceneCB, sizeof(sceneCB));

            // Transition resources
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

            // Set descriptor heaps and tables
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RasterRenderRootSig::Num; i++)
            {
                uint32_t heapIndex = (RasterDescriptorIndexStart + i);
                renderContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
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
    GraphicsContext& renderContext = GraphicsContext::Begin("Composite Pass");
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
            CompositeConstantBuffer compositeCB;
            {
                // The following multipliers let us select which buffer to display
                // See the CompositePass shader for more details.
                const XMFLOAT4 bufferOn  = XMFLOAT4(1, 1, 1, 1);
                const XMFLOAT4 bufferOff = XMFLOAT4(0, 0, 0, 0);
                for (int i = 0; i < ARRAY_SIZE(compositeCB.TextureMultipliers); i++)
                {
                    if (SelectedBufferIndex == i || SelectedBufferIndex == 0)
                    {
                        compositeCB.TextureMultipliers[i] = bufferOn;
                    }
                    else
                    {
                        compositeCB.TextureMultipliers[i] = bufferOff;
                    }
                }

                if (SelectedBufferIndex == 0)
                {
                    compositeCB.CompositeMultipliers[0] = bufferOn;
                    compositeCB.CompositeMultipliers[1] = bufferOff;
                }
                else
                {
                    compositeCB.CompositeMultipliers[0] = bufferOff;
                    compositeCB.CompositeMultipliers[1] = bufferOn;
                }

                // Setup direct/indirect multipliers
                compositeCB.DirectIndirectLightMult = XMFLOAT2(TheUserInputData.GpuDirectLightMult, TheUserInputData.GpuIndirectLightMult);
            }
            RasterCompositeConstantBuffer.Upload(&compositeCB, sizeof(compositeCB));

            // Transition resources
            //renderContext.TransitionResource(RasterCompositeConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
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
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RasterRenderRootSig::Num; i++)
            {
                uint32_t heapIndex = (RasterDescriptorIndexStart + i);
                renderContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
            }

            // Setup rest of the pipeline
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
