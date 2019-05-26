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

#include "DescriptorHeapStack.h"
#include "RenderDevice.h"

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

DescriptorHeapStack::DescriptorHeapStack(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT nodeMask)
{
    

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type           = type;
    desc.Flags          = (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask       = nodeMask;

    RenderDevice::Get().GetD3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DescriptorHeap));

    DescriptorSize          = RenderDevice::Get().GetD3DDevice()->GetDescriptorHandleIncrementSize(type);
    DescriptorHeapCpuBase   = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeapPtr DescriptorHeapStack::GetDescriptorHeap()
{
    return DescriptorHeap;
}

// ----------------------------------------------------------------------------------------------------------------------------

void DescriptorHeapStack::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, UINT& descriptorHeapIndex)
{
    descriptorHeapIndex = DescriptorsAllocated;
    cpuHandle           = CD3DX12_CPU_DESCRIPTOR_HANDLE(DescriptorHeapCpuBase, descriptorHeapIndex, DescriptorSize);
    CpuHandles.push_back(cpuHandle);
    DescriptorsAllocated++;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT RealtimeEngine::DescriptorHeapStack::AllocateTexture2DSrv(ID3D12Resource* resource, DXGI_FORMAT format, UINT shader4ComponentMapping)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels     = 1;
    srvDesc.Format                  = format;
    srvDesc.Shader4ComponentMapping = shader4ComponentMapping;
    RenderDevice::Get().GetD3DDevice()->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::GetCount()
{
    return DescriptorsAllocated;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferSrv(ID3D12Resource* resource, UINT numElements, D3D12_SRV_DIMENSION viewDimension, D3D12_BUFFER_SRV_FLAGS flags, DXGI_FORMAT format, UINT shader4ComponentMapping)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension               = viewDimension;
    srvDesc.Format                      = format;
    srvDesc.Buffer.Flags                = flags;
    srvDesc.Buffer.StructureByteStride  = 0;
    srvDesc.Buffer.FirstElement         = 0;
    srvDesc.Buffer.NumElements          = numElements;
    srvDesc.Shader4ComponentMapping     = shader4ComponentMapping;
    RenderDevice::Get().GetD3DDevice()->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferSrvRaytracing(D3D12_GPU_VIRTUAL_ADDRESS Location, D3D12_SRV_DIMENSION viewDimension, D3D12_BUFFER_SRV_FLAGS flags, DXGI_FORMAT format, UINT shader4ComponentMapping)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension                            = viewDimension;
    srvDesc.Buffer.Flags                             = flags;
    srvDesc.Format                                   = format;
    srvDesc.Shader4ComponentMapping                  = shader4ComponentMapping;
    srvDesc.RaytracingAccelerationStructure.Location = Location;
    RenderDevice::Get().GetD3DDevice()->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferCbv(D3D12_GPU_VIRTUAL_ADDRESS location, UINT size)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = location;
    cbvDesc.SizeInBytes    = size;
    RenderDevice::Get().GetD3DDevice()->CreateConstantBufferView(&cbvDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

UINT DescriptorHeapStack::AllocateBufferUav(ID3D12Resource & resource, D3D12_UAV_DIMENSION viewDimension, D3D12_BUFFER_UAV_FLAGS flags, DXGI_FORMAT format)
{
    UINT                        descriptorHeapIndex;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    AllocateDescriptor(cpuHandle, descriptorHeapIndex);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension       = viewDimension;
    uavDesc.Buffer.NumElements  = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
    uavDesc.Buffer.Flags        = flags;
    uavDesc.Format              = format;
    RenderDevice::Get().GetD3DDevice()->CreateUnorderedAccessView(&resource, nullptr, &uavDesc, cpuHandle);

    return descriptorHeapIndex;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapStack::GetGpuHandle(UINT descriptorIndex)
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(DescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, DescriptorSize);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapStack::GetCpuHandle(UINT descriptorIndex)
{
    return CpuHandles[descriptorIndex];
}