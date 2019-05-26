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

#include "GpuBuffer.h"
#include "RenderDevice.h"
#include "CommandContext.h"

// ----------------------------------------------------------------------------------------------------------------------------

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

GpuBuffer::GpuBuffer() 
    : BufferSize(0), ElementCount(0), ElementSize(0)
{
    ResourceFlags                  = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 0;
    HeapProps.VisibleNodeMask      = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

GpuBuffer::~GpuBuffer()
{
    Destroy();
}

// ----------------------------------------------------------------------------------------------------------------------------

void GpuBuffer::Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize, const void* initialData, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_FLAGS resourceFlags)
{
    Destroy();

    ElementCount  = numElements;
    ElementSize   = elementSize;
    BufferSize    = numElements * elementSize;
    UsageState    = usageState;
    ResourceFlags = resourceFlags;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();
    ASSERT_SUCCEEDED( 
        RenderDevice::Get().GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
        &ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&ResourcePtr)));

    GpuVirtualAddress = ResourcePtr->GetGPUVirtualAddress();

    if (initialData)
    {
        CommandContext::InitializeBuffer(*this, initialData, BufferSize);
    }

    #ifdef RELEASE
        (name);
    #else
        ResourcePtr->SetName(name.c_str());
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void GpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t heapOffset, uint32_t numElements, uint32_t elementSize, const void* initialData)
{
    ElementCount = numElements;
    ElementSize  = elementSize;
    BufferSize   = numElements * elementSize;
    UsageState     = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();
    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreatePlacedResource(pBackingHeap, heapOffset, &ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&ResourcePtr)));

    GpuVirtualAddress = ResourcePtr->GetGPUVirtualAddress();

    if (initialData)
    {
        CommandContext::InitializeBuffer(*this, initialData, BufferSize);
    }

    #ifdef RELEASE
        (name);
    #else
        ResourcePtr->SetName(name.c_str());
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t baseVertexIndex /*= 0*/) const
{
    size_t offset = baseVertexIndex * ElementSize;
    return VertexBufferView(offset, (uint32_t)(BufferSize - offset), ElementSize);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const
{
    D3D12_VERTEX_BUFFER_VIEW vbView;
    vbView.BufferLocation = GpuVirtualAddress + offset;
    vbView.SizeInBytes    = size;
    vbView.StrideInBytes  = stride;

    return vbView;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t startIndex /*= 0*/) const
{
    size_t offset = startIndex * ElementSize;
    return IndexBufferView(offset, (uint32_t)(BufferSize - offset), ElementSize == 4);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t offset, uint32_t size, bool b32Bit) const
{
    D3D12_INDEX_BUFFER_VIEW ibView;
    ibView.BufferLocation  = GpuVirtualAddress + offset;
    ibView.Format          = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes     = size;

    return ibView;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_RESOURCE_DESC GpuBuffer::DescribeBuffer()
{
    ASSERT(BufferSize != 0);

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment          = 0;
    Desc.DepthOrArraySize   = 1;
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = ResourceFlags;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Height             = 1;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels          = 1;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width              = (UINT64)BufferSize;

    return Desc;
}

// ----------------------------------------------------------------------------------------------------------------------------

UploadBuffer::UploadBuffer()
{
    HeapProps.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 0;
    HeapProps.VisibleNodeMask      = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void UploadBuffer::Map(void** ppBuffer)
{
    ResourcePtr->Map(0, nullptr, ppBuffer);
}

// ----------------------------------------------------------------------------------------------------------------------------

void UploadBuffer::Unmap()
{
    ResourcePtr->Unmap(0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ReadbackBuffer::Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize)
{
    Destroy();

    ElementCount = numElements;
    ElementSize  = elementSize;
    BufferSize   = numElements * elementSize;
    UsageState   = D3D12_RESOURCE_STATE_COPY_DEST;

    // Create a readback buffer large enough to hold all texel data
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                  = D3D12_HEAP_TYPE_READBACK;
    HeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask      = 1;
    HeapProps.VisibleNodeMask       = 1;

    // Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Width              = BufferSize;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&ResourcePtr)));

    GpuVirtualAddress = ResourcePtr->GetGPUVirtualAddress();

    #ifdef RELEASE
        (name);
    #else
        ResourcePtr->SetName(name.c_str());
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void* ReadbackBuffer::Map(void)
{
    void* pMemory;
    ResourcePtr->Map(0, &CD3DX12_RANGE(0, BufferSize), &pMemory);

    return pMemory;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ReadbackBuffer::Unmap(void)
{
    ResourcePtr->Unmap(0, &CD3DX12_RANGE(0, 0));
}
