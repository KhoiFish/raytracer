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

        static void          DestroyAll(void);
        void                 SetRootSignature(const RootSignature& BindMappings) { RootSig = &BindMappings; }
        const RootSignature& GetRootSignature(void) const                        { return *RootSig; }
        ID3D12PipelineState* GetPipelineStateObject(void) const                  { return PSOObject; }

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

        void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
        void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
        void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
        void SetSampleMask(UINT SampleMask);
        void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
        void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
        void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
        void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
        void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

        void SetVertexShader(const void* Binary, size_t Size) { PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetPixelShader(const void* Binary, size_t Size) { PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetGeometryShader(const void* Binary, size_t Size) { PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetHullShader(const void* Binary, size_t Size) { PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetDomainShader(const void* Binary, size_t Size) { PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

        void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.VS = Binary; }
        void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.PS = Binary; }
        void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.GS = Binary; }
        void SetHullShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.HS = Binary; }
        void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.DS = Binary; }

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

        void SetComputeShader(const void* Binary, size_t Size)     { PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
        void SetComputeShader(const D3D12_SHADER_BYTECODE& Binary) { PSODesc.CS = Binary; }
        void Finalize();

    private:

        D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
    };

}
