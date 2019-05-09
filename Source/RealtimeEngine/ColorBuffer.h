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
#include "PixelBuffer.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class CommandContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    struct Color
    {
        Color(float r, float g, float b, float a)
        {
            Data[0] = r;
            Data[1] = g;
            Data[2] = b;
            Data[3] = a;
        }

        float        R() const                   { return Data[0]; }
        float        G() const                   { return Data[1]; }
        float        B() const                   { return Data[2]; }
        float        A() const                   { return Data[3]; }
        float&       operator[] (uint32_t index) { return Data[index]; }
        const float* GetPtr()                    { return Data; }

        float Data[4];
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ColorBuffer : public PixelBuffer
    {
    public:

        ColorBuffer(Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f));

        void                                CreateFromSwapChain(const string_t& name, ID3D12Resource* baseResource);
        void                                Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        void                                CreateArray(const string_t& name, uint32_t width, uint32_t height, uint32_t arrayCount, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        void                                SetClearColor(Color clearColor);
        void                                SetMsaaMode(uint32_t numColorSamples, uint32_t numCoverageSamples);
        void                                GenerateMipMaps(CommandContext & context);

        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetSRV(void) const          { return SRVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetRTV(void) const          { return RTVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetUAV(void) const          { return UAVHandle[0]; }
        Color                               GetClearColor(void) const   { return ClearColor; }

    protected:

        D3D12_RESOURCE_FLAGS                CombineResourceFlags(void) const;
        void                                CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format, uint32_t arraySize, uint32_t numMips = 1);

    protected:

        Color                               ClearColor;
        D3D12_CPU_DESCRIPTOR_HANDLE         SRVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE         RTVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE         UAVHandle[12];
        uint32_t                            numMipMaps;
        uint32_t                            FragmentCount;
        uint32_t                            SampleCount;
    };
}
