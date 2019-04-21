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
#include "RenderDevice.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{

    class RootParameter
    {
    public:

        RootParameter()
        {
            RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
        }

        ~RootParameter()
        {
            Clear();
        }

    public:

        void Clear();
        void InitAsConstants(UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0);

        const D3D12_ROOT_PARAMETER& operator() (void) const
        {
            return RootParam;
        }

    protected:

        D3D12_ROOT_PARAMETER RootParam;
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class RootSignature
    {
    public:

        RootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0)
            : Finalized(false), NumParameters(NumRootParams)
        {
            Reset(NumRootParams, NumStaticSamplers);
        }

        ~RootSignature()
        {
        }

    public:

        static void  DestroyAll(void);
        void         Reset(UINT NumRootParams, UINT NumStaticSamplers = 0);
        void         InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
        void         Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

    public:

        RootParameter& operator[] (size_t EntryIndex)
        {
            assert(EntryIndex < NumParameters);
            return ParamArray.get()[EntryIndex];
        }

        const RootParameter& operator[] (size_t EntryIndex) const
        {
            assert(EntryIndex < NumParameters);
            return ParamArray.get()[EntryIndex];
        }

        ID3D12RootSignature* GetSignature() const
        {
            return Signature;
        }

    protected:

        bool                                          Finalized;
        uint32_t                                      NumParameters;
        uint32_t                                      NumSamplers;
        uint32_t                                      NumInitializedStaticSamplers;
        uint32_t                                      DescriptorTableBitMap;
        uint32_t                                      SamplerTableBitMap;
        uint32_t                                      DescriptorTableSize[16];
        std::unique_ptr<RootParameter[]>              ParamArray;
        std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]>  SamplerArray;
        ID3D12RootSignature*                          Signature;
    };
}
