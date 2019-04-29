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

#include "PixelBuffer.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class DepthBuffer : public PixelBuffer
    {
    public:
        DepthBuffer(float clearDepth = 0.0f, uint8_t clearStencil = 0);

        void                                Create(const string_t& name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        void                                Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numSamples, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV() const                  { return DSVHandle[0]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_DepthReadOnly() const    { return DSVHandle[1]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_StencilReadOnly() const  { return DSVHandle[2]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_ReadOnly() const         { return DSVHandle[3]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDepthSRV() const             { return DepthSRVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetStencilSRV() const           { return StencilSRVHandle; }
        float                               GetClearDepth() const           { return ClearDepth; }
        uint8_t                             GetClearStencil() const         { return ClearStencil; }

    private:

        void                                CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format);

    private:

        float                               ClearDepth;
        uint8_t                             ClearStencil;
        D3D12_CPU_DESCRIPTOR_HANDLE         DSVHandle[4];
        D3D12_CPU_DESCRIPTOR_HANDLE         DepthSRVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE         StencilSRVHandle;
    };
}

