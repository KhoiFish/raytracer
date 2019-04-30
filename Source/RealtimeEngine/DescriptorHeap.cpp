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
