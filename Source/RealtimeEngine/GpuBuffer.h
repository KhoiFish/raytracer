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
#include "GpuResource.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class CommandContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    class GpuBuffer : public GpuResource
    {
    public:

        virtual ~GpuBuffer();

    public:

        void                                Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize, const void* initialData = nullptr, D3D12_RESOURCE_STATES usageState = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        void                                CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t heapOffset, uint32_t numElements, uint32_t elementSize, const void* initialData = nullptr);
        D3D12_CPU_DESCRIPTOR_HANDLE         CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

        D3D12_GPU_VIRTUAL_ADDRESS           RootConstantBufferView() const;
        D3D12_VERTEX_BUFFER_VIEW            VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const;
        D3D12_VERTEX_BUFFER_VIEW            VertexBufferView(size_t baseVertexIndex = 0) const;
        D3D12_INDEX_BUFFER_VIEW             IndexBufferView(size_t offset, uint32_t Size, bool b32Bit = false) const;
        D3D12_INDEX_BUFFER_VIEW             IndexBufferView(size_t startIndex = 0) const;

        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetUAV() const           { return UAV; }
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetSRV() const           { return SRV; }
        size_t                              GetBufferSize() const    { return BufferSize; }
        uint32_t                            GetElementCount() const  { return ElementCount; }
        uint32_t                            GetElementSize() const   { return ElementSize; }

    protected:

        friend class ComputeContext;

        GpuBuffer(void);

        D3D12_RESOURCE_DESC                 DescribeBuffer();
        virtual void                        CreateDerivedViews() = 0;

    protected:

        D3D12_CPU_DESCRIPTOR_HANDLE         UAV;
        D3D12_CPU_DESCRIPTOR_HANDLE         SRV;
        size_t                              BufferSize;
        uint32_t                            ElementCount;
        uint32_t                            ElementSize;
        D3D12_RESOURCE_FLAGS                ResourceFlags;
        D3D12_HEAP_PROPERTIES               HeapProps;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class UploadBuffer : public GpuBuffer
    {
    public:

        UploadBuffer();

        virtual void    CreateDerivedViews() override {}
        void            Map(void** ppBuffer);
        void            Unmap();
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class NoViewBuffer : public GpuBuffer
    {
    public:

        virtual void CreateDerivedViews() override {}
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ByteAddressBuffer : public GpuBuffer
    {
    public:

        virtual void CreateDerivedViews() override;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class IndirectArgsBuffer : public ByteAddressBuffer
    {
    public:

        IndirectArgsBuffer(void) {}
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class StructuredBuffer : public GpuBuffer
    {
    public:

        virtual void                         Destroy() override;
        virtual void                         CreateDerivedViews(void) override;

        ByteAddressBuffer&                   GetCounterBuffer();
        const D3D12_CPU_DESCRIPTOR_HANDLE&   GetCounterSRV(CommandContext& context);
        const D3D12_CPU_DESCRIPTOR_HANDLE&   GetCounterUAV(CommandContext& context);

    private:

        ByteAddressBuffer                    CounterBuffer;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class TypedBuffer : public GpuBuffer
    {
    public:

        TypedBuffer(DXGI_FORMAT format) : DataFormat(format) {}
        virtual void CreateDerivedViews() override;

    protected:

        DXGI_FORMAT DataFormat;
    };
}
