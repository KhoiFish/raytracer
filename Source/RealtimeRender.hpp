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
    RealtimeRenderingRegisters_LinearSampler    = 0,
    RealtimeRenderingRegisters_AnisoSampler     = 1,
};

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRealtimeRenderer()
{
    // Delete old one, if it exists
    if (RealtimeDescriptorHeap != nullptr)
    {
        delete RealtimeDescriptorHeap;
        RealtimeDescriptorHeap = nullptr;
    }

    // Create heap for descriptors
    RealtimeDescriptorHeap = new DescriptorHeapStack(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);

    RealtimeDescriptorHeap->AllocateTexture2DSrv(
        AmbientOcclusionOutput[1].GetResource(),
        RaytracingBufferType,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    RealtimeDescriptorHeap->AllocateTexture2DSrv(
        CPURaytracerTex.GetResource(),
        CPURaytracerTexType,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    for (int i = 0; i < DeferredBufferType_Num; i++)
    {
        RealtimeDescriptorHeap->AllocateTexture2DSrv(
            DeferredBuffers[i].GetResource(),
            DeferredBuffersRTTypes[i],
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRealtimePipeline()
{
    CpuResultsBufferIndex = RealtimeRenderingRegisters_Texture2 + 1;   

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
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture5, 1);
        RealtimeRootSignature[RealtimeRenderingRootIndex_Texture6].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RealtimeRenderingRegisters_Texture6, 1);
        RealtimeRootSignature.InitStaticSampler(RealtimeRenderingRegisters_LinearSampler, linearSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.InitStaticSampler(RealtimeRenderingRegisters_AnisoSampler, anisoSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
        RealtimeRootSignature.Finalize("RealtimeRender", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // This will create descriptors
        OnResizeRealtimeRenderer();
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

        RealtimeGeometryPassPSO.SetRootSignature(RealtimeRootSignature);
        RealtimeGeometryPassPSO.SetRasterizerState(rasterizerDesc);
        RealtimeGeometryPassPSO.SetBlendState(blendDisable);
        RealtimeGeometryPassPSO.SetDepthStencilState(depthStateReadWrite);
        RealtimeGeometryPassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
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
        depthDisabledState.DepthWriteMask                       = D3D12_DEPTH_WRITE_MASK_ZERO;
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
        RealtimeCompositePassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
        RealtimeCompositePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimeCompositePassPSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RealtimeCompositePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGeometryPass()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("RenderRealtimeResults");
    {
        renderContext.SetRootSignature(RealtimeRootSignature);
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
            renderContext.SetPipelineState(RealtimeGeometryPassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
        renderContext.SetRootSignature(RealtimeRootSignature);
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
            }

            // Transition resources
            renderContext.TransitionResource(CPURaytracerTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(AmbientOcclusionOutput[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            for (int i = 0; i < DeferredBufferType_Num; i++)
            {
                renderContext.TransitionResource(DeferredBuffers[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ, true);

            // Set descriptor heaps and tables
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RealtimeDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RealtimeDescriptorHeap->GetCount(); i++)
            {
                uint32_t rootIndex = RealtimeRenderingRootIndex_Texture0 + i;
                renderContext.SetDescriptorTable(rootIndex, RealtimeDescriptorHeap->GetGpuHandle(i));
            }

            // Draw
            renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(sceneCb), &sceneCb);
            renderContext.SetRenderTarget(RenderDevice::Get().GetRenderTarget().GetRTV(), RenderDevice::Get().GetDepthStencil().GetDSV());
            renderContext.SetPipelineState(RealtimeCompositePassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
        // Don't render light shapes
        if (TheRenderScene->GetRenderSceneList()[i]->Hitable->IsALightShape())
        {
            continue;
        }

        SceneConstantBuffer matrices;
        SetupSceneConstantBuffer(TheRenderScene->GetRenderSceneList()[i]->WorldMatrix, matrices);

        RealtimeEngine::Texture* defaultTexture = RealtimeEngine::TextureManager::LoadFromFile(DefaultTextureName);
        RealtimeEngine::Texture* diffuseTex = (TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture != nullptr) ? TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture : defaultTexture;

        renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(matrices), &matrices);

        renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RenderDevice::Get().GetDefaultDescriptorHeapStack().GetDescriptorHeap());
        renderContext.SetDescriptorTable(RealtimeRenderingRootIndex_Texture0, diffuseTex->GetGpuHandle());

        renderContext.SetVertexBuffer(0, TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.IndexBufferView());
        renderContext.DrawIndexed((uint32_t)TheRenderScene->GetRenderSceneList()[i]->Indices.size());
    }
}
