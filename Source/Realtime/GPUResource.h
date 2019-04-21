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

// ----------------------------------------------------------------------------------------------------------------------------

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

// ----------------------------------------------------------------------------------------------------------------------------

class GpuResource
{
public:

    GpuResource() :
        GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        UserAllocatedMemory(nullptr),
        UsageState(D3D12_RESOURCE_STATE_COMMON),
        TransitioningState((D3D12_RESOURCE_STATES)-1)
    {}

    GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
        GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        UserAllocatedMemory(nullptr),
        ResourcePtr(pResource),
        UsageState(CurrentState),
        TransitioningState((D3D12_RESOURCE_STATES)-1)
    {}

    virtual void Destroy()
    {
        ResourcePtr = nullptr;
        GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        if (UserAllocatedMemory != nullptr)
        {
            VirtualFree(UserAllocatedMemory, 0, MEM_RELEASE);
            UserAllocatedMemory = nullptr;
        }
    }

    ID3D12Resource*            operator->()                  { return ResourcePtr.Get(); }
    const ID3D12Resource*      operator->() const            { return ResourcePtr.Get(); }
    ID3D12Resource*            GetResource()                 { return ResourcePtr.Get(); }
    const ID3D12Resource*      GetResource() const           { return ResourcePtr.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS  GetGpuVirtualAddress() const  { return GpuVirtualAddress; }

protected:

    Microsoft::WRL::ComPtr<ID3D12Resource>   ResourcePtr;
    D3D12_RESOURCE_STATES                    UsageState;
    D3D12_RESOURCE_STATES                    TransitioningState;
    D3D12_GPU_VIRTUAL_ADDRESS                GpuVirtualAddress;
    void*                                    UserAllocatedMemory;
};
