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

#include "LinearAllocator.h"
#include "CommandList.h"
#include "RenderDevice.h"
#include <thread>

using namespace RealtimeEngine;
using namespace std;

// ----------------------------------------------------------------------------------------------------------------------------

LinearAllocatorType         LinearAllocatorPageManager::AutoType = kGpuExclusive;
LinearAllocatorPageManager  LinearAllocator::PageManager[2];

// ----------------------------------------------------------------------------------------------------------------------------

LinearAllocatorPageManager::LinearAllocatorPageManager()
{
    AllocationType   = AutoType;
    AutoType         = (LinearAllocatorType)(AutoType + 1);
 
    RTL_ASSERT(AutoType <= kNumAllocatorTypes);
}

// ----------------------------------------------------------------------------------------------------------------------------

LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
{
    lock_guard<mutex> lockGuard(Mutex);

    while (!RetiredPages.empty() && CommandListManager::Get().IsFenceComplete(RetiredPages.front().first))
    {
        AvailablePages.push(RetiredPages.front().second);
        RetiredPages.pop();
    }

    LinearAllocationPage* pagePtr = nullptr;

    if (!AvailablePages.empty())
    {
        pagePtr = AvailablePages.front();
        AvailablePages.pop();
    }
    else
    {
        pagePtr = CreateNewPage();
        PagePool.emplace_back(pagePtr);
    }

    return pagePtr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void LinearAllocatorPageManager::DiscardPages(uint64_t fenceValue, const vector<LinearAllocationPage*>& usedPages)
{
    lock_guard<mutex> lockGuard(Mutex);
    for (auto iter = usedPages.begin(); iter != usedPages.end(); ++iter)
    {
        RetiredPages.push(make_pair(fenceValue, *iter));
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void LinearAllocatorPageManager::FreeLargePages(uint64_t fenceValue, const vector<LinearAllocationPage*>& largePages)
{
    lock_guard<mutex> lockGuard(Mutex);

    while (!DeletionQueue.empty() && CommandListManager::Get().IsFenceComplete(DeletionQueue.front().first))
    {
        delete DeletionQueue.front().second;
        DeletionQueue.pop();
    }

    for (auto iter = largePages.begin(); iter != largePages.end(); ++iter)
    {
        (*iter)->Unmap();
        DeletionQueue.push(make_pair(fenceValue, *iter));
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void LinearAllocatorPageManager::Destroy(void)
{
    PagePool.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage(size_t pageSize)
{
    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask     = 1;
    heapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment           = 0;
    resourceDesc.Height              = 1;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_RESOURCE_STATES defaultUsage;
    if (AllocationType == kGpuExclusive)
    {
        heapProps.Type     = D3D12_HEAP_TYPE_DEFAULT;
        resourceDesc.Width = pageSize == 0 ? kGpuAllocatorPageSize : pageSize;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        defaultUsage       = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
        heapProps.Type     = D3D12_HEAP_TYPE_UPLOAD;
        resourceDesc.Width = pageSize == 0 ? kCpuAllocatorPageSize : pageSize;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        defaultUsage       = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    ID3D12Resource* pBuffer;
    RTL_HRESULT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, defaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)));

    pBuffer->SetName(L"LinearAllocator Page");

    return new LinearAllocationPage(pBuffer, defaultUsage);
}

// ----------------------------------------------------------------------------------------------------------------------------

void LinearAllocator::CleanupUsedPages(uint64_t fenceID)
{
    if (CurPage != nullptr)
    {
        RetiredPages.push_back(CurPage);
        CurPage = nullptr;
        CurOffset = 0;
    }

    PageManager[AllocationType].DiscardPages(fenceID, RetiredPages);
    RetiredPages.clear();

    PageManager[AllocationType].FreeLargePages(fenceID, LargePageList);
    LargePageList.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

void LinearAllocator::DestroyAll()
{
    PageManager[0].Destroy();
    PageManager[1].Destroy();
}

// ----------------------------------------------------------------------------------------------------------------------------

DynAlloc LinearAllocator::AllocateLargePage(size_t sizeInBytes)
{
    LinearAllocationPage* oneOff = PageManager[AllocationType].CreateNewPage(sizeInBytes);
    LargePageList.push_back(oneOff);

    DynAlloc ret(*oneOff, 0, sizeInBytes);
    ret.DataPtr    = oneOff->CpuVirtualAddress;
    ret.GpuAddress = oneOff->GpuVirtualAddress;

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

LinearAllocator::LinearAllocator(LinearAllocatorType type) 
    : AllocationType(type), PageSize(0), CurOffset(~(size_t)0), CurPage(nullptr)
{
    RTL_ASSERT(type > kInvalidAllocator && type < kNumAllocatorTypes);
    PageSize = (type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
}

// ----------------------------------------------------------------------------------------------------------------------------

DynAlloc LinearAllocator::Allocate(size_t sizeInBytes, size_t alignment)
{
    const size_t alignmentMask = alignment - 1;

    // Assert that it's a power of two.
    RTL_ASSERT((alignmentMask & alignment) == 0);

    // Align the allocation
    const size_t alignedSize = AlignUpWithMask(sizeInBytes, alignmentMask);

    if (alignedSize > PageSize)
    {
        return AllocateLargePage(alignedSize);
    }

    CurOffset = AlignUp(CurOffset, alignment);

    if (CurOffset + alignedSize > PageSize)
    {
        RTL_ASSERT(CurPage != nullptr);
        RetiredPages.push_back(CurPage);
        CurPage = nullptr;
    }

    if (CurPage == nullptr)
    {
        CurPage   = PageManager[AllocationType].RequestPage();
        CurOffset = 0;
    }

    DynAlloc ret(*CurPage, CurOffset, alignedSize);
    ret.DataPtr    = (uint8_t*)CurPage->CpuVirtualAddress + CurOffset;
    ret.GpuAddress = CurPage->GpuVirtualAddress + CurOffset;

    CurOffset += alignedSize;

    return ret;
}
