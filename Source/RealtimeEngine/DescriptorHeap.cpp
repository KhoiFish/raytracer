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

#include "DescriptorHeap.h"
#include "RenderDevice.h"

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

std::mutex                                                DescriptorAllocator::AllocationMutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorAllocator::DescriptorHeapPool;

// ----------------------------------------------------------------------------------------------------------------------------

void DescriptorAllocator::DestroyAll()
{
    DescriptorHeapPool.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    std::lock_guard<std::mutex> lockGuard(AllocationMutex);

    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type           = type;
    Desc.NumDescriptors = NumDescriptorsPerHeap;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NodeMask       = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
    DescriptorHeapPool.emplace_back(pHeap);

    return pHeap.Get();
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(uint32_t count)
{
    if (CurrentHeap == nullptr || RemainingFreeHandles < count)
    {
        CurrentHeap = RequestNewHeap(Type);
        CurrentHandle = CurrentHeap->GetCPUDescriptorHandleForHeapStart();
        RemainingFreeHandles = NumDescriptorsPerHeap;

        if (DescriptorSize == 0)
        {
            DescriptorSize = RenderDevice::Get().GetD3DDevice()->GetDescriptorHandleIncrementSize(Type);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE ret = CurrentHandle;
    CurrentHandle.ptr += count * DescriptorSize;
    RemainingFreeHandles -= count;

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

void UserDescriptorHeap::Create(const string_t& debugHeapName)
{
    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(Heap.ReleaseAndGetAddressOf())));

    #ifdef RELEASE
        (void)debugHeapName;
    #else
        Heap->SetName(MakeWStr(debugHeapName).c_str());
    #endif

    DescriptorSize     = RenderDevice::Get().GetD3DDevice()->GetDescriptorHandleIncrementSize(HeapDesc.Type);
    NumFreeDescriptors = HeapDesc.NumDescriptors;
    FirstHandle        = DescriptorHandle( Heap->GetCPUDescriptorHandleForHeapStart(),  Heap->GetGPUDescriptorHandleForHeapStart() );
    NextFreeHandle     = FirstHandle;
}

// ----------------------------------------------------------------------------------------------------------------------------

DescriptorHandle UserDescriptorHeap::Alloc(uint32_t count)
{
    ASSERT(HasAvailableSpace(count), "Descriptor Heap out of space.  Increase heap size.");
    DescriptorHandle ret = NextFreeHandle;
    NextFreeHandle += count * DescriptorSize;

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool UserDescriptorHeap::ValidateHandle(const DescriptorHandle& dHandle) const
{
    if (dHandle.GetCpuHandle().ptr < FirstHandle.GetCpuHandle().ptr ||
        dHandle.GetCpuHandle().ptr >= FirstHandle.GetCpuHandle().ptr + HeapDesc.NumDescriptors * DescriptorSize)
    {
        return false;
    }

    if (dHandle.GetGpuHandle().ptr - FirstHandle.GetGpuHandle().ptr !=
        dHandle.GetCpuHandle().ptr - FirstHandle.GetCpuHandle().ptr)
    {
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

DescriptorHeapStack::DescriptorHeapStack(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type           = type;
    desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask       = nodeMask;

    RenderDevice::Get().GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DescriptorHeap));

    DescriptorSize          = RenderDevice::Get().GetD3DDevice()->GetDescriptorHandleIncrementSize(type);
    DescriptorHeapCpuBase   = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap& DescriptorHeapStack::GetDescriptorHeap()
{
    return *DescriptorHeap;
}

// ----------------------------------------------------------------------------------------------------------------------------

void DescriptorHeapStack::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT& descriptorHeapIndex)
{
    descriptorHeapIndex = DescriptorsAllocated;
    cpuHandle           = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeapCpuBase, descriptorHeapIndex, DescriptorSize);
    DescriptorsAllocated++;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferSrv(ID3D12Resource& resource)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements      = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
    srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.Format                  = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    RenderDevice::Get().GetD3DDevice()->CreateShaderResourceView(&resource, &srvDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferUav(ID3D12Resource & resource)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements  = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
    uavDesc.Buffer.Flags        = D3D12_BUFFER_UAV_FLAG_RAW;
    uavDesc.Format              = DXGI_FORMAT_R32_TYPELESS;
    RenderDevice::Get().GetD3DDevice()->CreateUnorderedAccessView(&resource, nullptr, &uavDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapStack::GetGpuHandle(UINT descriptorIndex)
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, DescriptorSize);
}
