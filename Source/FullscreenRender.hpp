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

enum FullscreenQuadRootIndex
{
    FullscreenQuadRootIndex_Texture0 = 0,

    FullscreenQuadRootIndex_Num
};

enum FullscreenQuadRegisters
{
    FullscreenQuadRegisters_Texture0 = 0,
    FullscreenQuadRegisters_LinearSampler = 0
};

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
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\FullscreenQuad_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { RenderDevice::Get().GetBackBufferFormat() };

        FullscreenPipelineState.SetRootSignature(FullscreenQuadRootSignature);
        FullscreenPipelineState.SetRasterizerState(rasterizerDesc);
        FullscreenPipelineState.SetBlendState(blendDisable);
        FullscreenPipelineState.SetDepthStencilState(depthDisabledState);
        FullscreenPipelineState.SetInputLayout(RenderSceneVertexEx::InputElementCount, RenderSceneVertexEx::InputElements);
        FullscreenPipelineState.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        FullscreenPipelineState.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        FullscreenPipelineState.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        FullscreenPipelineState.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        FullscreenPipelineState.Finalize();
    }
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
        renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ, true);
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
