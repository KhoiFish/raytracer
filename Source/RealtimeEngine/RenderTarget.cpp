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

#include "RenderTarget.h"
#include "CommandContext.h"
#include "RenderDevice.h"

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

static inline uint32_t ComputeNumMips(uint32_t width, uint32_t height)
{
    uint32_t HighBit;
    _BitScanReverse((unsigned long*)& HighBit, width | height);
    return HighBit + 1;
}

// ----------------------------------------------------------------------------------------------------------------------------

static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;

    // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_R32G8X24_TYPELESS;

    // No Stencil
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_TYPELESS;

    // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_R24G8_TYPELESS;

    // 16-bit Z w/o Stencil
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_TYPELESS;

    default:
        return defaultFormat;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_UNORM;

    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

#ifdef _DEBUG
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_D16_UNORM:

    ASSERT(false, "Requested a UAV format for a depth stencil format.");
#endif

    default:
        return defaultFormat;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    // No Stencil
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;

    // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 16-bit Z w/o Stencil
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_D16_UNORM;

    default:
        return defaultFormat;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

    // No Stencil
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

    // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    // 16-bit Z w/o Stencil
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_UNORM;

    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

    // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static size_t BytesPerPixel(DXGI_FORMAT format)
{
    switch(format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 16;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 12;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 8;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 4;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 2;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_P8:
        return 1;

    default:
        return 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static D3D12_RESOURCE_DESC DescribeTex2D(uint32_t width, uint32_t height, uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS resourceFlags)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment          = 0;
    desc.DepthOrArraySize   = (UINT16)depthOrArraySize;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags              = resourceFlags;
    desc.Format             = GetBaseFormat(format);
    desc.Height             = (uint32_t)height;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.MipLevels          = (UINT16)numMips;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width              = (UINT64)width;

    return desc;
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreateTextureResource(GpuResource* pGpuResource, const string_t& name, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_CLEAR_VALUE* clearValue, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr, D3D12_RESOURCE_STATES usageStates)
{
    pGpuResource->Destroy();

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, usageStates, clearValue, IID_PPV_ARGS(&pGpuResource->GetResourceRef())));

    #ifndef RELEASE
        gpuResource->SetName(MakeWStr(name).c_str());
    #else
        (name);
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderTarget::RenderTarget()
    : numMipMaps(0), FragmentCount(1), SampleCount(1)
{
    SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    std::memset(UAVHandle, 0xFF, sizeof(UAVHandle));

    for (int i = 0; i < 4; i++)
    {
        ClearColor[i] = 0.0f;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format, uint32_t arraySize, uint32_t numMips)
{
    ASSERT(arraySize == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

    numMipMaps = numMips - 1;

    D3D12_RENDER_TARGET_VIEW_DESC       rtvDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC    uavDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC     srvDesc = {};

    rtvDesc.Format                  = format;
    uavDesc.Format                  = GetUAVFormat(format);
    srvDesc.Format                  = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (arraySize > 1)
    {
        rtvDesc.ViewDimension                   = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice         = 0;
        rtvDesc.Texture2DArray.FirstArraySlice  = 0;
        rtvDesc.Texture2DArray.ArraySize        = (uint32_t)arraySize;

        uavDesc.ViewDimension                   = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice         = 0;
        uavDesc.Texture2DArray.FirstArraySlice  = 0;
        uavDesc.Texture2DArray.ArraySize        = (uint32_t)arraySize;

        srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MipLevels        = numMips;
        srvDesc.Texture2DArray.MostDetailedMip  = 0;
        srvDesc.Texture2DArray.FirstArraySlice  = 0;
        srvDesc.Texture2DArray.ArraySize        = (uint32_t)arraySize;
    }
    else if (FragmentCount > 1)
    {
        rtvDesc.ViewDimension                   = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    else 
    {
        rtvDesc.ViewDimension                   = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice              = 0;

        uavDesc.ViewDimension                   = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice              = 0;

        srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels             = numMips;
        srvDesc.Texture2D.MostDetailedMip       = 0;
    }

    if (SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        RTVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        SRVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    ID3D12Resource* Resource = ResourcePtr.Get();

    // Create the render target view
    if (ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        device->CreateRenderTargetView(Resource, &rtvDesc, RTVHandle);
    }

    // Create the shader resource view
    device->CreateShaderResourceView(Resource, &srvDesc, SRVHandle);

    if (FragmentCount > 1)
    {
        return;
    }

    // Create the UAVs for each mip level (RWTexture2D)
    for (uint32_t i = 0; i < numMips; ++i)
    {
        if (UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            UAVHandle[i] = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        device->CreateUnorderedAccessView(Resource, nullptr, &uavDesc, UAVHandle[i]);

        uavDesc.Texture2D.MipSlice++;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::CreateFromSwapChain(const string_t& name, ID3D12Resource* baseResource)
{
    RTVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    AssociateWithResource(RenderDevice::Get().GetD3DDevice(), name, baseResource, D3D12_RESOURCE_STATE_PRESENT);
    RenderDevice::Get().GetD3DDevice()->CreateRenderTargetView(ResourcePtr.Get(), nullptr, RTVHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    numMips = (numMips == 0 ? ComputeNumMips(width, height) : numMips);    

    Width             = width;
    Height            = height;
    ArraySize         = 1;
    Format            = format;
    ResourceFlags     = CombineResourceFlags();
    UsageState        = D3D12_RESOURCE_STATE_COMMON;
    GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, ArraySize, numMips, format, ResourceFlags);
    resourceDesc.SampleDesc.Count   = FragmentCount;
    resourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = format;
    memcpy(&clearValue.Color, ClearColor, sizeof(ClearColor));

    CreateTextureResource(this, name, resourceDesc, &clearValue, vidMemPtr, UsageState);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format, 1, numMips);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::RenderTarget::CreateEx(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_CLEAR_VALUE* clearValue, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES usageStates, bool createViews)
{
    numMips = (numMips == 0 ? ComputeNumMips(width, height) : numMips);

    Width               = width;
    Height              = height;
    ArraySize           = 1;
    Format              = format;
    ResourceFlags       = resourceFlags;
    UsageState          = usageStates;
    GpuVirtualAddress   = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, ArraySize, numMips, format, resourceFlags);
    resourceDesc.SampleDesc.Count = FragmentCount;
    resourceDesc.SampleDesc.Quality = 0;

    CreateTextureResource(this, name, resourceDesc, clearValue, vidMemPtr, usageStates);

    if (createViews)
    {
        CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format, 1, numMips);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::CreateArray(const string_t& name, uint32_t width, uint32_t height, uint32_t arrayCount, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    Width               = width;
    Height              = height;
    ArraySize           = arrayCount;
    Format              = format;
    ResourceFlags       = CombineResourceFlags();
    UsageState          = D3D12_RESOURCE_STATE_COMMON;
    GpuVirtualAddress   = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, ArraySize, 1, format, ResourceFlags);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = format;
    memcpy(&clearValue.Color, ClearColor, sizeof(ClearColor));

    CreateTextureResource(this, name, resourceDesc, &clearValue, vidMemPtr, UsageState);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format, arrayCount, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::AssociateWithResource(ID3D12Device* device, const string_t& name, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState)
{
    ASSERT(resource != nullptr);
    D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

    ResourcePtr.Attach(resource);
    UsageState = currentState;
    Width      = (uint32_t)resourceDesc.Width;
    Height     = resourceDesc.Height;
    ArraySize  = resourceDesc.DepthOrArraySize;
    Format     = resourceDesc.Format;

    #ifndef RELEASE
        ResourcePtr->SetName(MakeWStr(name).c_str());
    #else
        (name);
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::SetClearColor(float clearColor[4])
{
    memcpy(&ClearColor, clearColor, sizeof(clearColor));
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderTarget::SetMsaaMode(uint32_t numColorSamples, uint32_t numCoverageSamples)
{
    ASSERT(numCoverageSamples >= numColorSamples);
    FragmentCount = numColorSamples;
    SampleCount   = numCoverageSamples;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_RESOURCE_FLAGS RenderTarget::CombineResourceFlags() const
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (flags == D3D12_RESOURCE_FLAG_NONE && FragmentCount == 1)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}


// ----------------------------------------------------------------------------------------------------------------------------

DepthTarget::DepthTarget(float clearDepth, uint8_t clearStencil)
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

void DepthTarget::Create(const string_t& name, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{    
    Width               = width;
    Height              = height;
    ArraySize           = 1;
    Format              = format;
    ResourceFlags       = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    UsageState          = D3D12_RESOURCE_STATE_COMMON;
    GpuVirtualAddress   = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, ArraySize, 1, format, ResourceFlags);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = clearValue.Color[3] = ClearDepth;
    clearValue.Format = format;

    CreateTextureResource(this, name, resourceDesc, &clearValue, vidMemPtr, UsageState);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DepthTarget::Create(const string_t& name, uint32_t width, uint32_t height, uint32_t samples, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{    
    Width               = width;
    Height              = height;
    ArraySize           = 1;
    Format              = format;
    ResourceFlags       = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    UsageState          = D3D12_RESOURCE_STATE_COMMON;
    GpuVirtualAddress   = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, ArraySize, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    resourceDesc.SampleDesc.Count = samples;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = clearValue.Color[3] = ClearDepth;
    clearValue.Format = format;

    CreateTextureResource(this, name, resourceDesc, &clearValue, vidMemPtr, UsageState);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DepthTarget::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format)
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
