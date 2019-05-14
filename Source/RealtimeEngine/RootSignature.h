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

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
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
        void InitAsConstants        (uint32_t reg, uint32_t numDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsConstantBuffer   (uint32_t reg, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsBufferSRV        (uint32_t reg, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsBufferUAV        (uint32_t reg, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsDescriptorTable  (uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void InitAsDescriptorRange  (D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t reg, uint32_t count, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void SetTableRange          (uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t reg, uint32_t count, uint32_t space = 0);

        const D3D12_ROOT_PARAMETER& operator() () const
        {
            return RootParam;
        }

    protected:

        D3D12_ROOT_PARAMETER RootParam;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RootSignature
    {
    public:

        RootSignature(uint32_t numRootParams = 0, uint32_t numStaticSamplers = 0)
            : Finalized(false), NumParameters(numRootParams)
        {
            Reset(numRootParams, numStaticSamplers);
        }

        ~RootSignature()
        {
        }

    public:

        static void     DestroyAll();
        void            Reset(uint32_t numRootParams, uint32_t numStaticSamplers = 0);
        void            InitStaticSampler(uint32_t reg, const D3D12_SAMPLER_DESC& nonStaticSamplerDesc, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void            Finalize(const string_t& name, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
        const string_t& GetName() const;

    public:

        RootParameter& operator[] (size_t entryIndex)
        {
            assert(entryIndex < NumParameters);
            return ParamArray.get()[entryIndex];
        }

        const RootParameter& operator[] (size_t entryIndex) const
        {
            assert(entryIndex < NumParameters);
            return ParamArray.get()[entryIndex];
        }

        ID3D12RootSignature* GetSignature() const
        {
            return Signature;
        }

        friend class DynamicDescriptorHeap;

    protected:

        bool                                          Finalized;
        string_t                                      Name;
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
