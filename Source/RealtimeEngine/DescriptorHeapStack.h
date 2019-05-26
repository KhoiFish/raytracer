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
    class DescriptorHeapStack
    {
    public:

        DescriptorHeapStack(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask);

        void                        AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT& descriptorHeapIndex);
        UINT                        AllocateTexture2DSrv(ID3D12Resource* resource, DXGI_FORMAT format, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
        UINT                        AllocateBufferSrv(ID3D12Resource* resource, UINT numElements, D3D12_SRV_DIMENSION viewDimension = D3D12_SRV_DIMENSION_BUFFER, D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_RAW, DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
        UINT                        AllocateBufferSrvRaytracing(D3D12_GPU_VIRTUAL_ADDRESS Location, D3D12_SRV_DIMENSION viewDimension = D3D12_SRV_DIMENSION_BUFFER, D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_RAW, DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
        UINT                        AllocateBufferCbv(D3D12_GPU_VIRTUAL_ADDRESS location, UINT size);
        UINT                        AllocateBufferUav(ID3D12Resource& resource, D3D12_UAV_DIMENSION viewDimension = D3D12_UAV_DIMENSION_BUFFER, D3D12_BUFFER_UAV_FLAGS flags = D3D12_BUFFER_UAV_FLAG_RAW, DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS);
        ID3D12DescriptorHeapPtr     GetDescriptorHeap();
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT descriptorIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT descriptorIndex);
        UINT                        GetCount();

    private:

        ID3D12DescriptorHeapPtr                     DescriptorHeap;
        UINT                                        DescriptorsAllocated = 0;
        UINT                                        DescriptorHeapMaxCount = 0;
        UINT                                        DescriptorSize;
        D3D12_CPU_DESCRIPTOR_HANDLE                 DescriptorHeapCpuBase;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>    CpuHandles;
    };
}
