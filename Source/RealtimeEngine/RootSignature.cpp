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

#include "RootSignature.h"
#include "RenderDevice.h"
#include <map>
#include <thread>
#include <mutex>

using namespace RealtimeEngine;
using namespace std;
using Microsoft::WRL::ComPtr;

// ----------------------------------------------------------------------------------------------------------------------------

static std::map< size_t, ComPtr<ID3D12RootSignature> > sRootSignatureHashMap;

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::DestroyAll()
{
    sRootSignatureHashMap.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::InitStaticSampler(uint32_t reg, const D3D12_SAMPLER_DESC& nonStaticSamplerDesc, D3D12_SHADER_VISIBILITY visibility)
{
    RTL_ASSERT(NumInitializedStaticSamplers < NumSamplers);
    D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc = SamplerArray[NumInitializedStaticSamplers++];

    staticSamplerDesc.Filter           = nonStaticSamplerDesc.Filter;
    staticSamplerDesc.AddressU         = nonStaticSamplerDesc.AddressU;
    staticSamplerDesc.AddressV         = nonStaticSamplerDesc.AddressV;
    staticSamplerDesc.AddressW         = nonStaticSamplerDesc.AddressW;
    staticSamplerDesc.MipLODBias       = nonStaticSamplerDesc.MipLODBias;
    staticSamplerDesc.MaxAnisotropy    = nonStaticSamplerDesc.MaxAnisotropy;
    staticSamplerDesc.ComparisonFunc   = nonStaticSamplerDesc.ComparisonFunc;
    staticSamplerDesc.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSamplerDesc.MinLOD           = nonStaticSamplerDesc.MinLOD;
    staticSamplerDesc.MaxLOD           = nonStaticSamplerDesc.MaxLOD;
    staticSamplerDesc.ShaderRegister   = reg;
    staticSamplerDesc.RegisterSpace    = 0;
    staticSamplerDesc.ShaderVisibility = visibility;

    if (staticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
        staticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
        staticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
    {
        if (
            // Transparent Black
            nonStaticSamplerDesc.BorderColor[0] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[1] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[2] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[3] == 0.0f ||
            // Opaque Black
            nonStaticSamplerDesc.BorderColor[0] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[1] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[2] == 0.0f &&
            nonStaticSamplerDesc.BorderColor[3] == 1.0f ||
            // Opaque White
            nonStaticSamplerDesc.BorderColor[0] == 1.0f &&
            nonStaticSamplerDesc.BorderColor[1] == 1.0f &&
            nonStaticSamplerDesc.BorderColor[2] == 1.0f &&
            nonStaticSamplerDesc.BorderColor[3] == 1.0f)
        {
            RenderDebugPrintf("Sampler border color does not match static sampler limitations");
        }

        if (nonStaticSamplerDesc.BorderColor[3] == 1.0f)
        {
            if (nonStaticSamplerDesc.BorderColor[0] == 1.0f)
            {
                staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            }
            else
            {
                staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            }
        }
        else
        {
            staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::Finalize(const string_t & name, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    RTL_ASSERT(NumInitializedStaticSamplers == NumSamplers);
    if (Finalized)
    {
        return;
    }

    Name = name;

    size_t hashCode;
    D3D12_ROOT_SIGNATURE_DESC rootDesc;
    {
        rootDesc.NumParameters     = NumParameters;
        rootDesc.pParameters       = (const D3D12_ROOT_PARAMETER*)ParamArray.get();
        rootDesc.NumStaticSamplers = NumSamplers;
        rootDesc.pStaticSamplers   = (const D3D12_STATIC_SAMPLER_DESC*)SamplerArray.get();
        rootDesc.Flags             = flags;
    }

    hashCode              = HashState(rootDesc.pStaticSamplers, NumSamplers, HashState(&rootDesc.Flags));
    DescriptorTableBitMap = 0;
    SamplerTableBitMap    = 0;
    for (uint32_t param = 0; param < NumParameters; ++param)
    {
        const D3D12_ROOT_PARAMETER& rootParam = rootDesc.pParameters[param];
        DescriptorTableSize[param] = 0;

        if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            RTL_ASSERT(rootParam.DescriptorTable.pDescriptorRanges != nullptr);

            hashCode = HashState(rootParam.DescriptorTable.pDescriptorRanges, rootParam.DescriptorTable.NumDescriptorRanges, hashCode);

            // We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
            if (rootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
            {
                SamplerTableBitMap |= (1 << param);
            }
            else
            {
                DescriptorTableBitMap |= (1 << param);
            }

            for (uint32_t TableRange = 0; TableRange < rootParam.DescriptorTable.NumDescriptorRanges; ++TableRange)
            {
                DescriptorTableSize[param] += rootParam.DescriptorTable.pDescriptorRanges[TableRange].NumDescriptors;
            }
        }
        else
        {
            hashCode = HashState(&rootParam, 1, hashCode);
        }
    }

    ID3D12RootSignature * *rsRef = nullptr;
    bool firstCompile = false;
    {
        static mutex s_HashMapMutex;
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = sRootSignatureHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sRootSignatureHashMap.end())
        {
            rsRef = sRootSignatureHashMap[hashCode].GetAddressOf();
            firstCompile = true;
        }
        else
        {
            rsRef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

        RTL_HRESULT_SUCCEEDED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()));
        RTL_HRESULT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&Signature)));

        Signature->SetName(MakeWStr(name).c_str());

        sRootSignatureHashMap[hashCode].Attach(Signature);
        RTL_ASSERT(*rsRef == Signature);
    }
    else
    {
        while (*rsRef == nullptr)
        {
            this_thread::yield();
        }
        Signature = *rsRef;
    }

    Finalized = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::Reset(uint32_t numRootParams, uint32_t numStaticSamplers)
{
    if (numRootParams > 0)
    {
        ParamArray.reset(new RootParameter[numRootParams]);
    }
    else
    {
       ParamArray = nullptr;
    }

    if (numStaticSamplers > 0)
    {
        SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers]);
    }
    else
    {
       SamplerArray = nullptr;
    }

    NumParameters                = numRootParams;
    NumSamplers                  = numStaticSamplers;
    NumInitializedStaticSamplers = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

const string_t& RootSignature::GetName() const
{
    return Name;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::Clear()
{
    if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
    {
        delete[] RootParam.DescriptorTable.pDescriptorRanges;
    }

    RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsConstants(uint32_t reg, uint32_t numDwords, D3D12_SHADER_VISIBILITY visibility)
{
    RootParam.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    RootParam.ShaderVisibility          = visibility;
    RootParam.Constants.Num32BitValues  = numDwords;
    RootParam.Constants.ShaderRegister  = reg;
    RootParam.Constants.RegisterSpace   = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsConstantBuffer(uint32_t reg, D3D12_SHADER_VISIBILITY visibility)
{
    RootParam.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParam.ShaderVisibility          = visibility;
    RootParam.Descriptor.ShaderRegister = reg;
    RootParam.Descriptor.RegisterSpace  = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsBufferSRV(uint32_t reg, D3D12_SHADER_VISIBILITY visibility)
{
    RootParam.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    RootParam.ShaderVisibility          = visibility;
    RootParam.Descriptor.ShaderRegister = reg;
    RootParam.Descriptor.RegisterSpace  = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsBufferUAV(uint32_t reg, D3D12_SHADER_VISIBILITY visibility)
{
    RootParam.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    RootParam.ShaderVisibility          = visibility;
    RootParam.Descriptor.ShaderRegister = reg;
    RootParam.Descriptor.RegisterSpace  = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t reg, uint32_t count, uint32_t space, D3D12_SHADER_VISIBILITY visibility)
{
    InitAsDescriptorTable(1, visibility);
    SetTableRange(0, type, reg, count, space);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsDescriptorTable(uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility)
{
    RootParam.ParameterType                         = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParam.ShaderVisibility                      = visibility;
    RootParam.DescriptorTable.NumDescriptorRanges   = rangeCount;
    RootParam.DescriptorTable.pDescriptorRanges     = new D3D12_DESCRIPTOR_RANGE[rangeCount];
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::SetTableRange(uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t reg, uint32_t count, uint32_t space)
{
    D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(RootParam.DescriptorTable.pDescriptorRanges + rangeIndex);

    range->RangeType                            = type;
    range->NumDescriptors                       = count;
    range->BaseShaderRegister                   = reg;
    range->RegisterSpace                        = space;
    range->OffsetInDescriptorsFromTableStart    = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}
