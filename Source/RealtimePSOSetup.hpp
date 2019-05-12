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
    RealtimeRenderingRegisters_ConstantBuffer0 = 0,
    RealtimeRenderingRegisters_ConstantBuffer1 = 1,
    RealtimeRenderingRegisters_Texture0 = 0,
    RealtimeRenderingRegisters_Texture1 = 1,
    RealtimeRenderingRegisters_Texture2 = 2,
    RealtimeRenderingRegisters_Texture3 = 3,
    RealtimeRenderingRegisters_Texture4 = 4,
    RealtimeRenderingRegisters_LinearSampler = 0,
    RealtimeRenderingRegisters_AnisoSampler = 1,
};

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
        depthStateReadWrite.DepthWriteMask                      = D3D12_DEPTH_WRITE_MASK_ZERO;
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
        RealtimeCompositePassPSO.SetInputLayout(RenderVertex::InputElementCount, RenderVertex::InputElements);
        RealtimeCompositePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RealtimeCompositePassPSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RealtimeCompositePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RealtimeCompositePassPSO.Finalize();
    }
}