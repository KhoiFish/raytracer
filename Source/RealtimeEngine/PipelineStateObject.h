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
#include <map>
#include <array>

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class PSO
    {
    public:

        PSO() : RootSig(nullptr) {}

        static void                     DestroyAll();
        virtual void                    SetRootSignature(const RootSignature& bindMappings) { RootSig = &bindMappings; }
        virtual const RootSignature&    GetRootSignature() const                            { return *RootSig; }
        virtual ID3D12PipelineState*    GetPipelineStateObject() const                      { return PSOObject; }
        virtual void                    Finalize() {}

    protected:

        const RootSignature* RootSig;
        ID3D12PipelineState* PSOObject;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class GraphicsPSO : public PSO
    {
        friend class CommandContext;

    public:

        GraphicsPSO();

        void SetBlendState(const D3D12_BLEND_DESC& blendDesc);
        void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);
        void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
        void SetSampleMask(uint32_t sampleMask);
        void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
        void SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, uint32_t msaaCount = 1, uint32_t msaaQuality = 0);
        void SetRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat, uint32_t msaaCount = 1, uint32_t msaaQuality = 0);
        void SetInputLayout(uint32_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
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

        void Finalize() override;

    private:

        D3D12_GRAPHICS_PIPELINE_STATE_DESC               PSODesc;
        std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC>  InputLayouts;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ComputePSO : public PSO
    {
        friend class CommandContext;

    public:

        ComputePSO();

        void SetComputeShader(const void* binary, size_t size)     { PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
        void SetComputeShader(const D3D12_SHADER_BYTECODE& binary) { PSODesc.CS = binary; }
        void Finalize() override;

    private:

        D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RaytracingPSO : public PSO
    {
    public:

        RaytracingPSO();

        void            SetDxilLibrary(ID3DBlobPtr pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount);
        void            SetHitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name);
        void            SetShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes);
        void            SetPipelineConfig(uint32_t maxTraceRecursionDepth);
        void            AddLocalRootSignature(const std::string& localRootKeyName, RootSignature* pLocalRootSignature);
        void            AddExportAssociationWithLocalRootSignature(const WCHAR* exportNames[], uint32_t exportCount, const std::string& localRootKeyName);
        void            AddExportAssociationWithShaderConfig(const WCHAR* exportNames[], uint32_t exportCount);
        RootSignature&  GetGlobalRootSignature();        
        void            Finalize() override;

    private:

        int32_t         AddSuboject(D3D12_STATE_SUBOBJECT_TYPE type, const void* pDesc);
        void            AddExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, D3D12_STATE_SUBOBJECT* pSubobjectState);

    private:

        struct LocalRootSignatureData
        {
            int32_t                 LocalRootSignatureStateIndex;
            RootSignature*          LocalRootSignature;
            ID3D12RootSignature*    LocalRootSignatureInterfacePtr;
        };

    private:

        friend class CommandContext;

        ID3D12StateObjectPtr                                    RaytracingPipelineStateObject;

        static const int                                        MAX_SUBOBJECTS = 128;
        uint32_t                                                NumSubobjects = 0;
        D3D12_STATE_SUBOBJECT                                   Subobjects[MAX_SUBOBJECTS];

        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>    ExportAssoicationsList;
        std::map<std::string, LocalRootSignatureData*>          LocalRootSignatureDataList;
        
        int32_t                                                 DxilStateIndex = -1;
        D3D12_DXIL_LIBRARY_DESC                                 DxilLibDesc = {};
        ID3DBlobPtr                                             DxilLibShaderBlob;
        std::vector<D3D12_EXPORT_DESC>                          DxilLibExportDesc;
        std::vector<std::wstring>                               DxilLibExportName;

        int32_t                                                 HitProgramStateIndex = -1;
        std::wstring                                            HitProgramExportName;
        D3D12_HIT_GROUP_DESC                                    HitProgramDesc;
       
        int32_t                                                 GlobalRootSignatureStateIndex = -1;
        RootSignature                                           GlobalRootSignature;
        ID3D12RootSignature*                                    GlobalRootSignatureInterfacePtr;

        int32_t                                                 ShaderConfigStateIndex = -1;
        D3D12_RAYTRACING_SHADER_CONFIG                          ShaderConfig;

        int32_t                                                 PipelineConfigStateIndex = -1;
        D3D12_RAYTRACING_PIPELINE_CONFIG                        PipelineConfig;
    };
}
