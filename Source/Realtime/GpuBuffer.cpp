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

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

GpuBuffer::GpuBuffer() 
    : BufferSize(0), ElementCount(0), ElementSize(0)
{
    ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    UAV.ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    SRV.ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

// ----------------------------------------------------------------------------------------------------------------------------

GpuBuffer::~GpuBuffer()
{
    Destroy();
}

// ----------------------------------------------------------------------------------------------------------------------------

void GpuBuffer::Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize, const void* initialData)
{
    Destroy();

    ElementCount = numElements;
    ElementSize  = elementSize;
    BufferSize   = numElements * elementSize;
    UsageState     = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();
    ASSERT_SUCCEEDED( 
        RenderDevice::Get()->GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
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

    CreateDerivedViews();
}

// ----------------------------------------------------------------------------------------------------------------------------

void GpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t heapOffset, uint32_t numElements, uint32_t elementSize, const void* initialData)
{
    ElementCount = numElements;
    ElementSize  = elementSize;
    BufferSize   = numElements * elementSize;
    UsageState     = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();
    ASSERT_SUCCEEDED(RenderDevice::Get()->GetD3DDevice()->CreatePlacedResource(pBackingHeap, heapOffset, &ResourceDesc, UsageState, nullptr, IID_PPV_ARGS(&ResourcePtr)));

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

    CreateDerivedViews();
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_VIRTUAL_ADDRESS GpuBuffer::RootConstantBufferView() const
{
    return GpuVirtualAddress;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GpuBuffer::Create(const std::wstring& name, uint32_t numElements, uint32_t elementSize, EsramAllocator&, const void* initialData)
{
    Create(name, numElements, elementSize, initialData);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE GpuBuffer::CreateConstantBufferView(uint32_t offset, uint32_t size) const
{
    ASSERT(offset + size <= BufferSize);
    size = AlignUp(size, 16);

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    CBVDesc.BufferLocation  = GpuVirtualAddress + (size_t)offset;
    CBVDesc.SizeInBytes     = size;

    D3D12_CPU_DESCRIPTOR_HANDLE hCBV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    RenderDevice::Get()->GetD3DDevice()->CreateConstantBufferView(&CBVDesc, hCBV);

    return hCBV;
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

void ByteAddressBuffer::CreateDerivedViews()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension            = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format                   = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Shader4ComponentMapping  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements       = (UINT)BufferSize / 4;
    srvDesc.Buffer.Flags             = D3D12_BUFFER_SRV_FLAG_RAW;

    if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        SRV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    RenderDevice::Get()->GetD3DDevice()->CreateShaderResourceView(ResourcePtr.Get(), &srvDesc, SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format              = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.Buffer.NumElements  = (UINT)BufferSize / 4;
    UAVDesc.Buffer.Flags        = D3D12_BUFFER_UAV_FLAG_RAW;

    if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        UAV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    RenderDevice::Get()->GetD3DDevice()->CreateUnorderedAccessView(ResourcePtr.Get(), nullptr, &UAVDesc, UAV );
}

// ----------------------------------------------------------------------------------------------------------------------------

void StructuredBuffer::Destroy()
{
    CounterBuffer.Destroy();
    GpuBuffer::Destroy();
}

// ----------------------------------------------------------------------------------------------------------------------------

void StructuredBuffer::CreateDerivedViews()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements         = ElementCount;
    srvDesc.Buffer.StructureByteStride = ElementSize;
    srvDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

    if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        SRV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    RenderDevice::Get()->GetD3DDevice()->CreateShaderResourceView(ResourcePtr.Get(), &srvDesc, SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format                      = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.NumElements          = ElementCount;
    uavDesc.Buffer.StructureByteStride  = ElementSize;
    uavDesc.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;

    CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

    if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        UAV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    RenderDevice::Get()->GetD3DDevice()->CreateUnorderedAccessView(ResourcePtr.Get(), CounterBuffer.GetResource(), &uavDesc, UAV);
}

// ----------------------------------------------------------------------------------------------------------------------------

ByteAddressBuffer& StructuredBuffer::GetCounterBuffer()
{
    return CounterBuffer;
}

// ----------------------------------------------------------------------------------------------------------------------------

void TypedBuffer::CreateDerivedViews()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format                  = DataFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements      = ElementCount;
    srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_NONE;

    if (SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        SRV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    RenderDevice::Get()->GetD3DDevice()->CreateShaderResourceView(ResourcePtr.Get(), &srvDesc, SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension      = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format             = DataFormat;
    uavDesc.Buffer.NumElements = ElementCount;
    uavDesc.Buffer.Flags       = D3D12_BUFFER_UAV_FLAG_NONE;

    if (UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        UAV = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    RenderDevice::Get()->GetD3DDevice()->CreateUnorderedAccessView(ResourcePtr.Get(), nullptr, &uavDesc, UAV);
}

// ----------------------------------------------------------------------------------------------------------------------------

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterSRV(CommandContext& Context)
{
    Context.TransitionResource(CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
    return CounterBuffer.GetSRV();
}

// ----------------------------------------------------------------------------------------------------------------------------

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterUAV(CommandContext& Context)
{
    Context.TransitionResource(CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return CounterBuffer.GetUAV();
}
