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

#include "Globals.h"
#include "DepthBuffer.h"
#include "DescriptorHeapStack.h"
#include "RenderDevice.h"

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

DepthBuffer::DepthBuffer(float clearDepth, uint8_t clearStencil)
    : ClearDepth(clearDepth), ClearStencil(clearStencil)
{
    DSVHandle[0].ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    DSVHandle[1].ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    DSVHandle[2].ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    DSVHandle[3].ptr       = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    DepthSRVHandle.ptr     = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    StencilSRVHandle.ptr   = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

// ----------------------------------------------------------------------------------------------------------------------------

void DepthBuffer::Create( const string_t& name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr )
{
    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = clearValue.Color[3] = ClearDepth;
    clearValue.Format = format;

    CreateTextureResource(RenderDevice::Get().GetD3DDevice(), name, resourceDesc, &clearValue, vidMemPtr);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DepthBuffer::Create(const string_t& name, uint32_t width, uint32_t height, uint32_t samples, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    resourceDesc.SampleDesc.Count = samples;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = clearValue.Color[3] = ClearDepth;
    clearValue.Format = format;

    CreateTextureResource(RenderDevice::Get().GetD3DDevice(), name, resourceDesc, &clearValue, vidMemPtr);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DepthBuffer::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format)
{
    ID3D12Resource* resource = ResourcePtr.Get();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = GetDSVFormat(format);
    if (resource->GetDesc().SampleDesc.Count == 1)
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
    }
    else
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    if (DSVHandle[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        DSVHandle[0] = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        DSVHandle[1] = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    device->CreateDepthStencilView(resource, &dsvDesc, DSVHandle[0]);

    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    device->CreateDepthStencilView(resource, &dsvDesc, DSVHandle[1]);

    DXGI_FORMAT stencilReadFormat = GetStencilFormat(format);
    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (DSVHandle[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            DSVHandle[2] = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            DSVHandle[3] = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        device->CreateDepthStencilView(resource, &dsvDesc, DSVHandle[2]);

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        device->CreateDepthStencilView(resource, &dsvDesc, DSVHandle[3]);
    }
    else
    {
        DSVHandle[2] = DSVHandle[0];
        DSVHandle[3] = DSVHandle[1];
    }

    if (DepthSRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        DepthSRVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Create the shader resource view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = GetDepthFormat(format);
    if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
    }
    else
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    device->CreateShaderResourceView(resource, &srvDesc, DepthSRVHandle);

    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (StencilSRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            StencilSRVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        srvDesc.Format = stencilReadFormat;
        device->CreateShaderResourceView(resource, &srvDesc, StencilSRVHandle);
    }
}
