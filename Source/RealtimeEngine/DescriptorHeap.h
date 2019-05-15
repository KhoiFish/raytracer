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
#include <mutex>
#include <vector>
#include <queue>
#include <string>

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    // ----------------------------------------------------------------------------------------------------------------------------

    // This is an unbounded resource descriptor allocator.  It is intended to provide space for CPU-visible resource descriptors
    // as resources are created.  For those that need to be made shader-visible, they will need to be copied to a UserDescriptorHeap
    // or a DynamicDescriptorHeap.
    class DescriptorAllocator
    {
    public:

        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) : Type(type), DescriptorSize(0), RemainingFreeHandles(0), CurrentHeap(nullptr) {}

    public:

        D3D12_CPU_DESCRIPTOR_HANDLE   Allocate(uint32_t count);
        static void                   DestroyAll(void);

    protected:

        static ID3D12DescriptorHeap*  RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

    protected:

        static const uint32_t                                             NumDescriptorsPerHeap = 256;
        static std::mutex                                                 AllocationMutex;
        static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>  DescriptorHeapPool;

        D3D12_DESCRIPTOR_HEAP_TYPE                                        Type;
        ID3D12DescriptorHeap*                                             CurrentHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE                                       CurrentHandle;
        uint32_t                                                          DescriptorSize;
        uint32_t                                                          RemainingFreeHandles;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class DescriptorHandle
    {
    public:

        DescriptorHandle()
        {
            CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
            : CpuHandle(cpuHandle)
        {
            GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
            : CpuHandle(cpuHandle), GpuHandle(gpuHandle)
        {
        }

        DescriptorHandle operator+ (INT offsetScaledByDescriptorSize) const
        {
            DescriptorHandle ret = *this;
            ret += offsetScaledByDescriptorSize;
            return ret;
        }

    public:

        void operator += (INT offsetScaledByDescriptorSize)
        {
            if (CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            {
                CpuHandle.ptr += offsetScaledByDescriptorSize;
            }

            if (GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            {
                GpuHandle.ptr += offsetScaledByDescriptorSize;
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE  GetCpuHandle() const     { return CpuHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE  GetGpuHandle() const     { return GpuHandle; }
        bool                         IsNull() const           { return CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
        bool                         IsShaderVisible() const  { return GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

    private:

        D3D12_CPU_DESCRIPTOR_HANDLE  CpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE  GpuHandle;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class UserDescriptorHeap
    {
    public:

        UserDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxCount)
            : DescriptorSize(0), NumFreeDescriptors(0)
        {
            HeapDesc.Type           = type;
            HeapDesc.NumDescriptors = maxCount;
            HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            HeapDesc.NodeMask       = 1;
        }

        void                  Create(const string_t& debugHeapName);
        DescriptorHandle      Alloc(uint32_t count = 1);
        bool                  ValidateHandle(const DescriptorHandle& dHandle) const;

        ID3D12DescriptorHeap* GetHeapPointer() const                    { return Heap.Get(); }
        DescriptorHandle      GetHandleAtOffset(uint32_t offset) const  { return FirstHandle + offset * DescriptorSize; }
        bool                  HasAvailableSpace(uint32_t count) const   { return count <= NumFreeDescriptors; }

    private:

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>  Heap;
        D3D12_DESCRIPTOR_HEAP_DESC                    HeapDesc;
        uint32_t                                      DescriptorSize;
        uint32_t                                      NumFreeDescriptors;
        DescriptorHandle                              FirstHandle;
        DescriptorHandle                              NextFreeHandle;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class DescriptorHeapStack
    {
    public:

        DescriptorHeapStack(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask);

        void                        AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT& descriptorHeapIndex);
        UINT                        AllocateBufferSrv(ID3D12Resource& resource);
        UINT                        AllocateBufferUav(ID3D12Resource& resource);
        ID3D12DescriptorHeap&       GetDescriptorHeap();
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT descriptorIndex);

    private:

        CComPtr<ID3D12DescriptorHeap>   DescriptorHeap;
        UINT                            DescriptorsAllocated = 0;
        UINT                            DescriptorSize;
        D3D12_CPU_DESCRIPTOR_HANDLE     DescriptorHeapCpuBase;
    };
}
