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

using namespace yart;
using namespace std;
using Microsoft::WRL::ComPtr;

// ----------------------------------------------------------------------------------------------------------------------------

static std::map< size_t, ComPtr<ID3D12RootSignature> > sRootSignatureHashMap;

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::DestroyAll(void)
{
    sRootSignatureHashMap.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc, D3D12_SHADER_VISIBILITY Visibility)
{
    ASSERT(NumInitializedStaticSamplers < NumSamplers);
    D3D12_STATIC_SAMPLER_DESC& StaticSamplerDesc = SamplerArray[NumInitializedStaticSamplers++];

    StaticSamplerDesc.Filter           = NonStaticSamplerDesc.Filter;
    StaticSamplerDesc.AddressU         = NonStaticSamplerDesc.AddressU;
    StaticSamplerDesc.AddressV         = NonStaticSamplerDesc.AddressV;
    StaticSamplerDesc.AddressW         = NonStaticSamplerDesc.AddressW;
    StaticSamplerDesc.MipLODBias       = NonStaticSamplerDesc.MipLODBias;
    StaticSamplerDesc.MaxAnisotropy    = NonStaticSamplerDesc.MaxAnisotropy;
    StaticSamplerDesc.ComparisonFunc   = NonStaticSamplerDesc.ComparisonFunc;
    StaticSamplerDesc.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    StaticSamplerDesc.MinLOD           = NonStaticSamplerDesc.MinLOD;
    StaticSamplerDesc.MaxLOD           = NonStaticSamplerDesc.MaxLOD;
    StaticSamplerDesc.ShaderRegister   = Register;
    StaticSamplerDesc.RegisterSpace    = 0;
    StaticSamplerDesc.ShaderVisibility = Visibility;

    if (StaticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
        StaticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
        StaticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
    {
        WARN_ONCE_IF_NOT(
            // Transparent Black
            NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
            // Opaque Black
            NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
            NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
            // Opaque White
            NonStaticSamplerDesc.BorderColor[0] == 1.0f &&
            NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
            NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
            NonStaticSamplerDesc.BorderColor[3] == 1.0f,
            "Sampler border color does not match static sampler limitations");

        if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
        {
            if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
            {
                StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            }
            else
            {
                StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            }
        }
        else
        {
            StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::Finalize(const std::wstring & name, D3D12_ROOT_SIGNATURE_FLAGS Flags)
{
    ASSERT(NumInitializedStaticSamplers == NumSamplers);
    if (Finalized)
    {
        return;
    }

    size_t hashCode;
    D3D12_ROOT_SIGNATURE_DESC RootDesc;
    {
        RootDesc.NumParameters     = NumParameters;
        RootDesc.pParameters       = (const D3D12_ROOT_PARAMETER*)ParamArray.get();
        RootDesc.NumStaticSamplers = NumSamplers;
        RootDesc.pStaticSamplers   = (const D3D12_STATIC_SAMPLER_DESC*)SamplerArray.get();
        RootDesc.Flags             = Flags;
    }

    hashCode              = HashState(RootDesc.pStaticSamplers, NumSamplers, HashState(&RootDesc.Flags));
    DescriptorTableBitMap = 0;
    SamplerTableBitMap    = 0;
    for (UINT Param = 0; Param < NumParameters; ++Param)
    {
        const D3D12_ROOT_PARAMETER& RootParam = RootDesc.pParameters[Param];
        DescriptorTableSize[Param] = 0;

        if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            ASSERT(RootParam.DescriptorTable.pDescriptorRanges != nullptr);

            hashCode = HashState(RootParam.DescriptorTable.pDescriptorRanges, RootParam.DescriptorTable.NumDescriptorRanges, hashCode);

            // We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
            if (RootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
            {
                SamplerTableBitMap |= (1 << Param);
            }
            else
            {
                DescriptorTableBitMap |= (1 << Param);
            }

            for (UINT TableRange = 0; TableRange < RootParam.DescriptorTable.NumDescriptorRanges; ++TableRange)
            {
                DescriptorTableSize[Param] += RootParam.DescriptorTable.pDescriptorRanges[TableRange].NumDescriptors;
            }
        }
        else
        {
            hashCode = HashState(&RootParam, 1, hashCode);
        }
    }

    ID3D12RootSignature * *RSRef = nullptr;
    bool firstCompile = false;
    {
        static mutex s_HashMapMutex;
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = sRootSignatureHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sRootSignatureHashMap.end())
        {
            RSRef = sRootSignatureHashMap[hashCode].GetAddressOf();
            firstCompile = true;
        }
        else
        {
            RSRef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1, pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()));
        ASSERT_SUCCEEDED(RenderDevice::Get()->GetD3DDevice()->CreateRootSignature(1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&Signature)));

        Signature->SetName(name.c_str());

        sRootSignatureHashMap[hashCode].Attach(Signature);
        ASSERT(*RSRef == Signature);
    }
    else
    {
        while (*RSRef == nullptr)
        {
            this_thread::yield();
        }
        Signature = *RSRef;
    }

    Finalized = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootSignature::Reset(UINT NumRootParams, UINT NumStaticSamplers /*= 0*/)
{
    if (NumRootParams > 0)
    {
        ParamArray.reset(new RootParameter[NumRootParams]);
    }
    else
    {
       ParamArray = nullptr;
    }

    if (NumStaticSamplers > 0)
    {
        SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
    }
    else
    {
       SamplerArray = nullptr;
    }

    NumParameters                = NumRootParams;
    NumSamplers                  = NumStaticSamplers;
    NumInitializedStaticSamplers = 0;
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

void RootParameter::InitAsConstants(UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    RootParam.ShaderVisibility = Visibility;
    RootParam.Constants.Num32BitValues = NumDwords;
    RootParam.Constants.ShaderRegister = Register;
    RootParam.Constants.RegisterSpace = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParam.ShaderVisibility = Visibility;
    RootParam.Descriptor.ShaderRegister = Register;
    RootParam.Descriptor.RegisterSpace = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    RootParam.ShaderVisibility = Visibility;
    RootParam.Descriptor.ShaderRegister = Register;
    RootParam.Descriptor.RegisterSpace = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    RootParam.ShaderVisibility = Visibility;
    RootParam.Descriptor.ShaderRegister = Register;
    RootParam.Descriptor.RegisterSpace = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    InitAsDescriptorTable(1, Visibility);
    SetTableRange(0, Type, Register, Count);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
    RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParam.ShaderVisibility = Visibility;
    RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
    RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
}

// ----------------------------------------------------------------------------------------------------------------------------

void RootParameter::SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space /*= 0*/)
{
    D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);

    range->RangeType = Type;
    range->NumDescriptors = Count;
    range->BaseShaderRegister = Register;
    range->RegisterSpace = Space;
    range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}
