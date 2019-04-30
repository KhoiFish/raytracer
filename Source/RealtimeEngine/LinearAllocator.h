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
#include <vector>
#include <queue>
#include <mutex>

// ----------------------------------------------------------------------------------------------------------------------------

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    // ----------------------------------------------------------------------------------------------------------------------------

    struct DynAlloc
    {
        DynAlloc(GpuResource& baseResource, size_t thisOffset, size_t thisSize)
            : Buffer(baseResource), Offset(thisOffset), Size(thisSize) {}

        GpuResource&               Buffer;       // The D3D buffer associated with this memory.
        size_t                     Offset;       // Offset from start of buffer resource
        size_t                     Size;         // Reserved size of this allocation
        void*                      DataPtr;      // The CPU-writeable address
        D3D12_GPU_VIRTUAL_ADDRESS  GpuAddress;   // The GPU-visible address
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class LinearAllocationPage : public GpuResource
    {
    public:

        LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES usage)
            : GpuResource()
        {
            ResourcePtr.Attach(pResource);
            UsageState = usage;
            GpuVirtualAddress = ResourcePtr->GetGPUVirtualAddress();
            ResourcePtr->Map(0, nullptr, &CpuVirtualAddress);
        }

        ~LinearAllocationPage()
        {
            Unmap();
        }

        void Map()
        {
            if (CpuVirtualAddress == nullptr)
            {
                ResourcePtr->Map(0, nullptr, &CpuVirtualAddress);
            }
        }

        void Unmap()
        {
            if (CpuVirtualAddress != nullptr)
            {
                ResourcePtr->Unmap(0, nullptr);
                CpuVirtualAddress = nullptr;
            }
        }

        void*                       CpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS   GpuVirtualAddress;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    enum LinearAllocatorType
    {
        kInvalidAllocator = -1,
        kGpuExclusive     =  0,        // DEFAULT   GPU-writeable (via UAV)
        kCpuWritable      =  1,        // UPLOAD CPU-writeable (but write combined)

        kNumAllocatorTypes
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    enum
    {
        kGpuAllocatorPageSize = 0x10000,    // 64K
        kCpuAllocatorPageSize = 0x200000    // 2MB
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class LinearAllocatorPageManager
    {
    public:

        LinearAllocatorPageManager();

    public:

        LinearAllocationPage*                                    RequestPage();
        LinearAllocationPage*                                    CreateNewPage(size_t pageSize = 0);
        void                                                     DiscardPages(uint64_t fenceID, const std::vector<LinearAllocationPage*>& pages);
        void                                                     FreeLargePages(uint64_t fenceID, const std::vector<LinearAllocationPage*>& pages);
        void                                                     Destroy();

    private:

        static LinearAllocatorType                               AutoType;
        LinearAllocatorType                                      AllocationType;
        std::vector<std::unique_ptr<LinearAllocationPage> >      PagePool;
        std::queue<std::pair<uint64_t, LinearAllocationPage*> >  RetiredPages;
        std::queue<std::pair<uint64_t, LinearAllocationPage*> >  DeletionQueue;
        std::queue<LinearAllocationPage*>                        AvailablePages;
        std::mutex                                               Mutex;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class LinearAllocator
    {
    public:

        LinearAllocator(LinearAllocatorType type);

    public:

        DynAlloc                             Allocate(size_t sizeInBytes, size_t alignment = DEFAULT_ALIGN);
        void                                 CleanupUsedPages(uint64_t fenceID);
        static void                          DestroyAll();

    private:

        DynAlloc                             AllocateLargePage(size_t sizeInBytes);

    private:

        static LinearAllocatorPageManager    PageManager[2];
        LinearAllocatorType                  AllocationType;
        size_t                               PageSize;
        size_t                               CurOffset;
        LinearAllocationPage*                CurPage;
        std::vector<LinearAllocationPage*>   RetiredPages;
        std::vector<LinearAllocationPage*>   LargePageList;
    };

}