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

#pragma once

#include "Globals.h"
#include "RootSignature.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class PSO
    {
    public:

        PSO() : RootSig(nullptr) {}

        static void          DestroyAll();
        void                 SetRootSignature(const RootSignature& bindMappings) { RootSig = &bindMappings; }
        const RootSignature& GetRootSignature() const                            { return *RootSig; }
        ID3D12PipelineState* GetPipelineStateObject() const                      { return PSOObject; }

    protected:

        const RootSignature* RootSig;
        ID3D12PipelineState* PSOObject;
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{

    class GraphicsPSO : public PSO
    {
        friend class CommandContext;

    public:

        GraphicsPSO();

        void SetBlendState(const D3D12_BLEND_DESC& blendDesc);
        void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);
        void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
        void SetSampleMask(UINT sampleMask);
        void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
        void SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, UINT msaaCount = 1, UINT msaaQuality = 0);
        void SetRenderTargetFormats(UINT numRTVs, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat, UINT msaaCount = 1, UINT msaaQuality = 0);
        void SetInputLayout(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
        void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibProps);

        void SetVertexShader(const void* binary, size_t size)       { PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetPixelShader(const void* binary, size_t size)        { PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetGeometryShader(const void* binary, size_t size)     { PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetHullShader(const void* binary, size_t size)         { PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetDomainShader(const void* binary, size_t size)       { PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }

        void SetVertexShader(const D3D12_SHADER_BYTECODE& binary)   { PSODesc.VS = binary; }
        void SetPixelShader(const D3D12_SHADER_BYTECODE& binary)    { PSODesc.PS = binary; }
        void SetGeometryShader(const D3D12_SHADER_BYTECODE& binary) { PSODesc.GS = binary; }
        void SetHullShader(const D3D12_SHADER_BYTECODE& binary)     { PSODesc.HS = binary; }
        void SetDomainShader(const D3D12_SHADER_BYTECODE& binary)   { PSODesc.DS = binary; }

        void Finalize();

    private:

        D3D12_GRAPHICS_PIPELINE_STATE_DESC               PSODesc;
        std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC>  InputLayouts;
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class ComputePSO : public PSO
    {
        friend class CommandContext;

    public:

        ComputePSO();

        void SetComputeShader(const void* binary, size_t size)     { PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetComputeShader(const D3D12_SHADER_BYTECODE& binary) { PSODesc.CS = binary; }
        void Finalize();

    private:

        D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
    };

}
