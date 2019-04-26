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
        PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) {}

        uint32_t GetWidth(void) const { return m_Width; }
        uint32_t GetHeight(void) const { return m_Height; }
        uint32_t GetDepth(void) const { return m_ArraySize; }
        const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

        // Has no effect on Windows
        void SetBankRotation(uint32_t RotationAmount) { m_BankRotation = RotationAmount; }

    protected:

        D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

        void AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);

        void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
            D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
            D3D12_CLEAR_VALUE ClearValue, EsramAllocator& Allocator);

        static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
        static size_t BytesPerPixel(DXGI_FORMAT Format);

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_ArraySize;
        DXGI_FORMAT m_Format;
        uint32_t m_BankRotation;
    };
}
