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

#include "PipelineStateObject.h"
#include "RenderDevice.h"
#include <memory.h>
#include <map>
#include <thread>
#include <mutex>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace RealtimeEngine;

#define ALIGN(num, alignment) ((((num) + alignment - 1) / alignment) * alignment)

// ----------------------------------------------------------------------------------------------------------------------------

static map< size_t, ComPtr<ID3D12PipelineState> > sGraphicsPSOHashMap;
static map< size_t, ComPtr<ID3D12PipelineState> > sComputePSOHashMap;

// ----------------------------------------------------------------------------------------------------------------------------

void PSO::DestroyAll()
{
    sGraphicsPSOHashMap.clear();
    sComputePSOHashMap.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

GraphicsPSO::GraphicsPSO()
{
    ZeroMemory(&PSODesc, sizeof(PSODesc));
    PSODesc.NodeMask                = 1;
    PSODesc.SampleMask              = 0xFFFFFFFFu;
    PSODesc.SampleDesc.Count        = 1;
    PSODesc.InputLayout.NumElements = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
{
    PSODesc.BlendState = blendDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
    PSODesc.RasterizerState = rasterizerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
    PSODesc.DepthStencilState = depthStencilDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetSampleMask(uint32_t sampleMask)
{
    PSODesc.SampleMask = sampleMask;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    RTL_ASSERT_MSG(topologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    PSODesc.PrimitiveTopologyType = topologyType;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibProps)
{
    PSODesc.IBStripCutValue = ibProps;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, uint32_t msaaCount, uint32_t msaaQuality)
{
    SetRenderTargetFormats(1, &rtvFormat, dsvFormat, msaaCount, msaaQuality);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat, uint32_t msaaCount, uint32_t msaaQuality)
{
    RTL_ASSERT_MSG(numRTVs == 0 || rtvFormats != nullptr, "Null format array conflicts with non-zero length");

    for (uint32_t i = 0; i < numRTVs; ++i)
    {
        PSODesc.RTVFormats[i] = rtvFormats[i];
    }

    for (uint32_t i = numRTVs; i < PSODesc.NumRenderTargets; ++i)
    {
        PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }

    PSODesc.NumRenderTargets   = numRTVs;
    PSODesc.DSVFormat          = dsvFormat;
    PSODesc.SampleDesc.Count   = msaaCount;
    PSODesc.SampleDesc.Quality = msaaQuality;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetInputLayout(uint32_t numElements, const D3D12_INPUT_ELEMENT_DESC * pInputElementDescs)
{
    PSODesc.InputLayout.NumElements = numElements;

    if (numElements > 0)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * numElements);
        memcpy(NewElements, pInputElementDescs, numElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
    else
    {
        InputLayouts = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::Finalize()
{
    RTL_ASSERT(RootSig->GetSignature() != nullptr);

    // Make sure the root signature is finalized first
    PSODesc.pRootSignature = RootSig->GetSignature();
    PSODesc.InputLayout.pInputElementDescs = nullptr;
    PSODesc.InputLayout.pInputElementDescs = InputLayouts.get();

    size_t                hashCode = HashState(InputLayouts.get(), PSODesc.InputLayout.NumElements, HashState(&PSODesc));
    ID3D12PipelineState * *PSORef  = nullptr;
    bool firstCompile = false;
    {
        static mutex sHashMapMutex;
        lock_guard<mutex> CS(sHashMapMutex);
        auto iter = sGraphicsPSOHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sGraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = sGraphicsPSOHashMap[hashCode].GetAddressOf();
        }
        else
        {
            PSORef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        RTL_HRESULT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSOObject)));
        sGraphicsPSOHashMap[hashCode].Attach(PSOObject);
    }
    else
    {
        while (*PSORef == nullptr)
        {
            this_thread::yield();
        }
        PSOObject = *PSORef;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputePSO::Finalize()
{
    // Make sure the root signature is finalized first
    PSODesc.pRootSignature = RootSig->GetSignature();
    RTL_ASSERT(PSODesc.pRootSignature != nullptr);

    size_t                hashCode = HashState(&PSODesc);
    ID3D12PipelineState * *PSORef = nullptr;
    bool firstCompile = false;
    {
        static mutex sHashMapMutex;
        lock_guard<mutex> CS(sHashMapMutex);
        auto iter = sComputePSOHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = sComputePSOHashMap[hashCode].GetAddressOf();
        }
        else
        {
            PSORef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        RTL_HRESULT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSOObject)));
        sComputePSOHashMap[hashCode].Attach(PSOObject);
    }
    else
    {
        while (*PSORef == nullptr)
        {
            this_thread::yield();
        }
        PSOObject = *PSORef;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ComputePSO::ComputePSO()
{
    ZeroMemory(&PSODesc, sizeof(PSODesc));
    PSODesc.NodeMask = 1;
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::RaytracingPSO()
    : RaytracingPipelineStateObject(nullptr)
    , GlobalRootSignatureInterfacePtr(nullptr)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::~RaytracingPSO()
{
    if (RaytracingPipelineStateObject != nullptr)
    {
        RaytracingPipelineStateObject->Release();
        RaytracingPipelineStateObject = nullptr;
    }

    for (std::map<std::string, LocalRootSignatureData*>::iterator iter = LocalRootSignatureDataList.begin();
         iter != LocalRootSignatureDataList.end();
         ++iter)
    {
        delete iter->second;
        iter->second = nullptr;
    }

    for (size_t i = 0; i < HitGroupList.size(); i++)
    {
        delete HitGroupList[i];
        HitGroupList[i] = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

int32_t RaytracingPSO::AddSuboject(D3D12_STATE_SUBOBJECT_TYPE type, const void* pDesc)
{
    RTL_ASSERT(NumSubobjects < MAX_SUBOBJECTS);
    D3D12_STATE_SUBOBJECT& stateObject = Subobjects[NumSubobjects++];
    stateObject.Type  = type;
    stateObject.pDesc = pDesc;

    return int32_t(NumSubobjects - 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::SetDxilLibrary(ID3DBlobPtr pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount)
{
    DxilLibShaderBlob   = pBlob;
    DxilLibDesc         = {};

    DxilLibExportDesc.resize(entryPointCount);
    DxilLibExportName.resize(entryPointCount);
    if (pBlob)
    {
        DxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
        DxilLibDesc.DXILLibrary.BytecodeLength  = pBlob->GetBufferSize();
        DxilLibDesc.NumExports                  = entryPointCount;
        DxilLibDesc.pExports                    = DxilLibExportDesc.data();

        for (uint32_t i = 0; i < entryPointCount; i++)
        {
            DxilLibExportName[i] = entryPoint[i];

            DxilLibExportDesc[i].Name           = DxilLibExportName[i].c_str();
            DxilLibExportDesc[i].Flags          = D3D12_EXPORT_FLAG_NONE;
            DxilLibExportDesc[i].ExportToRename = nullptr;
        }
    }

    RTL_ASSERT(DxilStateIndex == -1);
    DxilStateIndex = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &DxilLibDesc);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::AddHitGroup(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& hitGroupName)
{
    HitGroupData* pHitGroup = new HitGroupData();
    pHitGroup->HitGroupName   = hitGroupName;
    pHitGroup->AHSExport      = (ahsExport != nullptr) ? ahsExport : L"";
    pHitGroup->CHSExport      = (chsExport != nullptr) ? chsExport : L"";

    pHitGroup->HitProgramDesc = {};
    pHitGroup->HitProgramDesc.HitGroupExport         = pHitGroup->HitGroupName.c_str();
    pHitGroup->HitProgramDesc.AnyHitShaderImport     = (ahsExport != nullptr) ? pHitGroup->AHSExport.c_str() : nullptr;
    pHitGroup->HitProgramDesc.ClosestHitShaderImport = (chsExport != nullptr) ? pHitGroup->CHSExport.c_str() : nullptr;

    pHitGroup->SubobjectIndex = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &pHitGroup->HitProgramDesc);

    HitGroupList.push_back(pHitGroup);
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12StateObjectPtr RealtimeEngine::RaytracingPSO::GetRaytracingPipelineStateObject() const
{
    return RaytracingPipelineStateObject.GetInterfacePtr();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::SetShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
{
    ShaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
    ShaderConfig.MaxPayloadSizeInBytes   = maxPayloadSizeInBytes;

    RTL_ASSERT(ShaderConfigStateIndex == -1);
    ShaderConfigStateIndex = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &ShaderConfig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::SetPipelineConfig(uint32_t maxTraceRecursionDepth)
{
    PipelineConfig.MaxTraceRecursionDepth = maxTraceRecursionDepth;

    RTL_ASSERT(PipelineConfigStateIndex == -1);
    PipelineConfigStateIndex = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &PipelineConfig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::AddLocalRootSignature(RootSignature* pLocalRootSignature)
{
    LocalRootSignatureData* pData = new LocalRootSignatureData();
    pData->LocalRootSignature             = pLocalRootSignature;
    pData->LocalRootSignatureInterfacePtr = pLocalRootSignature->GetSignature();
    pData->LocalRootSignatureStateIndex   = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &pData->LocalRootSignatureInterfacePtr);

    LocalRootSignatureDataList[pLocalRootSignature->GetName()] = pData;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::AddExportAssociationWithLocalRootSignature(const WCHAR* exportNames[], uint32_t exportCount, RootSignature* pLocalRootSignature)
{
    AddExportAssociation(exportNames, exportCount, &Subobjects[LocalRootSignatureDataList[pLocalRootSignature->GetName()]->LocalRootSignatureStateIndex]);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::AddExportAssociationWithShaderConfig(const WCHAR* exportNames[], uint32_t exportCount)
{
    AddExportAssociation(exportNames, exportCount, &Subobjects[ShaderConfigStateIndex]);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::AddExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, D3D12_STATE_SUBOBJECT* pSubobjectState)
{
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* pExportsAssociation = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    pExportsAssociation->NumExports             = exportCount;
    pExportsAssociation->pExports               = exportNames;
    pExportsAssociation->pSubobjectToAssociate  = pSubobjectState;

    ExportAssoicationsList.push_back(pExportsAssociation);
   
    AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, pExportsAssociation);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::Finalize()
{
    // Create subobject state for global signature
    {
        GlobalRootSignatureInterfacePtr = RootSig->GetSignature();
        GlobalRootSignatureStateIndex   = AddSuboject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &GlobalRootSignatureInterfacePtr);
    }

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects  = NumSubobjects;
    desc.pSubobjects    = Subobjects;
    desc.Type           = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    RTL_HRESULT_SUCCEEDED(RenderDevice::Get().GetD3DDeviceDXR()->CreateStateObject(&desc, IID_PPV_ARGS(&RaytracingPipelineStateObject)));

    // Build the shader tables
    RaygenShaderTable.Build(RaytracingPipelineStateObject);
    MissShaderTable.Build(RaytracingPipelineStateObject);
    HitGroupShaderTable.Build(RaytracingPipelineStateObject);
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::ShaderTable& RaytracingPSO::GetRaygenShaderTable()
{
    return RaygenShaderTable;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::ShaderTable& RaytracingPSO::GetMissShaderTable()
{
    return MissShaderTable;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::ShaderTable& RaytracingPSO::GetHitGroupShaderTable()
{
    return HitGroupShaderTable;
}

// ----------------------------------------------------------------------------------------------------------------------------

struct RaytracingPSO::ShaderTable::ShaderRecordData
{
    ShaderRecordData(const std::wstring& shaderName, const void* pData, uint32_t dataSize)
    {
        ShaderName           = shaderName;
        DataSize             = dataSize;
        ShaderTableEntrySize = ALIGN(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + DataSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        Data                 = new uint8_t[ShaderTableEntrySize];

        if (pData != nullptr && dataSize > 0)
        {
            // Copy data after shader identifier
            memcpy((Data + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES), pData, dataSize);
        }
    }

    ~ShaderRecordData()
    {
        if (Data != nullptr)
        {
            delete Data;
            Data = nullptr;
        }
    }

    std::wstring    ShaderName;
    uint8_t*        Data;
    uint32_t        DataSize;
    uint32_t        ShaderTableEntrySize;
};

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::ShaderTable::ShaderTable() : ShaderEntryStride(0), FirstDataRecordSize(0)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingPSO::ShaderTable::~ShaderTable()
{
    for (int i = 0; i < (int)ShaderDataList.size(); i++)
    {
        delete ShaderDataList[i];
        ShaderDataList[i] = nullptr;
    }
    ShaderDataList.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

uint32_t RaytracingPSO::ShaderTable::AddShaderRecordData(const std::wstring& shaderName, const void* pData, uint32_t dataSize)
{
    if (FirstDataRecordSize == 0)
    {
        FirstDataRecordSize = dataSize;
    }
    RTL_ASSERT(FirstDataRecordSize == dataSize);

    ShaderRecordData* pNewBlob = new ShaderRecordData(shaderName, pData, dataSize);
    ShaderDataList.push_back(pNewBlob);

    return uint32_t(ShaderDataList.size() - 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingPSO::ShaderTable::Build(ID3D12StateObjectPtr pPipelineState)
{
    // Compute total buffer size
    uint32_t totalBufferSize = 0;
    void* pTempBuffer = nullptr;
    for (int i = 0; i < (int)ShaderDataList.size(); i++)
    {
        totalBufferSize += ShaderDataList[i]->ShaderTableEntrySize;
    }

    // Allocate
    ShaderTableBuffer.Create(L"Shadertable", 1, totalBufferSize, nullptr, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);

    // Copy to buffer
    void* pBuffer = nullptr;
    ShaderTableBuffer.Map(&pBuffer);
    {
        uint8_t* pRunner = (uint8_t*)pBuffer;
        for (int i = 0; i < (int)ShaderDataList.size(); i++)
        {
            // Get shader identifer
            void* pShaderIdentifier;
            {
                MAKE_SMART_COM_PTR(ID3D12StateObjectProperties);
                ID3D12StateObjectPropertiesPtr pRtsoProps;
                pPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));
                pShaderIdentifier = pRtsoProps->GetShaderIdentifier(ShaderDataList[i]->ShaderName.c_str());
            }

            // Copy shader identifier into the shader record
            memcpy(ShaderDataList[i]->Data, pShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

            // Now copy to the record to the temp buffer
            memcpy(pRunner, ShaderDataList[i]->Data, ShaderDataList[i]->ShaderTableEntrySize);
            pRunner += ShaderDataList[i]->ShaderTableEntrySize;
        }
    }
    ShaderTableBuffer.Unmap();

    ShaderEntryStride = ShaderDataList[0]->ShaderTableEntrySize;
}
