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
#include "GPUResource.h"
#include "DescriptorHeapStack.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class CommandContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    class RenderTarget : public GpuResource
    {
    public:

        static void                         Init();
        static void                         Shutdown();

        uint32_t                            GetWidth(void) const        { return Width; }
        uint32_t                            GetHeight(void) const       { return Height; }
        uint32_t                            GetDepth(void) const        { return ArraySize; }
        const DXGI_FORMAT&                  GetFormat(void) const       { return Format; }

    protected:

        D3D12_CPU_DESCRIPTOR_HANDLE         AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);

    protected:

        uint32_t                            Width;
        uint32_t                            Height;
        uint32_t                            ArraySize;
        DXGI_FORMAT                         Format;
        D3D12_RESOURCE_FLAGS                ResourceFlags;
        static DescriptorHeapStack*         DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ColorTarget : public RenderTarget
    {
    public:

        ColorTarget();

        void                                CreateFromSwapChain(const string_t& name, ID3D12Resource* baseResource);
        void                                Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        void                                CreateEx(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_CLEAR_VALUE* clearValue = nullptr, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES usageStates = D3D12_RESOURCE_STATE_COMMON, bool createViews = true);
        void                                CreateArray(const string_t& name, uint32_t width, uint32_t height, uint32_t arrayCount, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetSRV(void) const          { return SRVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetRTV(void) const          { return RTVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetUAV(void) const          { return UAVHandle[0]; }
        const float*                        GetClearColor() const       { return ClearColor; }

        void                                SetClearColor(float clearColor[4]);
        void                                SetMsaaMode(uint32_t numColorSamples, uint32_t numCoverageSamples);

    private:

        D3D12_RESOURCE_FLAGS                CombineResourceFlags(void) const;
        void                                CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format, uint32_t arraySize, uint32_t numMips = 1);
        void                                AssociateWithResource(ID3D12Device* device, const string_t& name, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState);

    private:

        float                               ClearColor[4];
        D3D12_CPU_DESCRIPTOR_HANDLE         SRVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE         RTVHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE         UAVHandle[12];
        uint32_t                            numMipMaps;
        uint32_t                            FragmentCount;
        uint32_t                            SampleCount;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class DepthTarget : public RenderTarget
    {
    public:

        DepthTarget(float clearDepth = 0.0f, uint8_t clearStencil = 0);

        void                                Create(const string_t& name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        void                                Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numSamples, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV() const                      { return DSVHandle[0]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_DepthReadOnly() const        { return DSVHandle[1]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_StencilReadOnly() const      { return DSVHandle[2]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDSV_ReadOnly() const             { return DSVHandle[3]; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetDepthSRV() const                 { return DepthSRVHandle; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetStencilSRV() const               { return StencilSRVHandle; }
        float                               GetClearDepth() const               { return ClearDepth; }
        uint8_t                             GetClearStencil() const             { return ClearStencil; }

        void                                SetClearDepthValue(float value)     { ClearDepth = value; }
        void                                SetClearStencilValue(uint8_t value) { ClearStencil = value; }

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
