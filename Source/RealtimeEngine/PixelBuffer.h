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

#include "GpuResource.h"
#include "GpuBuffer.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class PixelBuffer : public GpuResource
    {
    public:
        PixelBuffer() : Width(0), Height(0), ArraySize(0), Format(DXGI_FORMAT_UNKNOWN) {}

        uint32_t            GetWidth(void) const    { return Width; }
        uint32_t            GetHeight(void) const   { return Height; }
        uint32_t            GetDepth(void) const    { return ArraySize; }
        const DXGI_FORMAT&  GetFormat(void) const   { return Format; }

    protected:

        D3D12_RESOURCE_DESC DescribeTex2D(uint32_t width, uint32_t height, uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, uint32_t flags);
        void                AssociateWithResource(ID3D12Device* device, const string_t& name, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState);
        void                CreateTextureResource(ID3D12Device* device, const string_t& name, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_CLEAR_VALUE clearValue, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        static DXGI_FORMAT  GetBaseFormat(DXGI_FORMAT format);
        static DXGI_FORMAT  GetUAVFormat(DXGI_FORMAT format);
        static DXGI_FORMAT  GetDSVFormat(DXGI_FORMAT format);
        static DXGI_FORMAT  GetDepthFormat(DXGI_FORMAT format);
        static DXGI_FORMAT  GetStencilFormat(DXGI_FORMAT format);
        static size_t       BytesPerPixel(DXGI_FORMAT format);

    protected:

        uint32_t            Width;
        uint32_t            Height;
        uint32_t            ArraySize;
        DXGI_FORMAT         Format;
    };
}
