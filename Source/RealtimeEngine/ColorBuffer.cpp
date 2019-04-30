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

#include "ColorBuffer.h"
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

ColorBuffer::ColorBuffer(Color clearColor)
    : ClearColor(clearColor), numMipMaps(0), FragmentCount(1), SampleCount(1)
{
    SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    std::memset(UAVHandle, 0xFF, sizeof(UAVHandle));
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format, uint32_t arraySize, uint32_t numMips)
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
    device->CreateRenderTargetView(Resource, &rtvDesc, RTVHandle);

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

void ColorBuffer::CreateFromSwapChain(const string_t& name, ID3D12Resource* baseResource)
{
    RTVHandle = RenderDevice::Get().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    AssociateWithResource(RenderDevice::Get().GetD3DDevice(), name, baseResource, D3D12_RESOURCE_STATE_PRESENT);
    RenderDevice::Get().GetD3DDevice()->CreateRenderTargetView(ResourcePtr.Get(), nullptr, RTVHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::Create(const string_t& name, uint32_t width, uint32_t height, uint32_t numMips, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    numMips = (numMips == 0 ? ComputeNumMips(width, height) : numMips);

    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, numMips, format, CombineResourceFlags());
    resourceDesc.SampleDesc.Count   = FragmentCount;
    resourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = format;
    clearValue.Color[0] = ClearColor.R();
    clearValue.Color[1] = ClearColor.G();
    clearValue.Color[2] = ClearColor.B();
    clearValue.Color[3] = ClearColor.A();

    CreateTextureResource(RenderDevice::Get().GetD3DDevice(), name, resourceDesc, clearValue, vidMemPtr);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format, 1, numMips);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::CreateArray(const string_t& name, uint32_t width, uint32_t height, uint32_t arrayCount, DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, arrayCount, 1, format, CombineResourceFlags());

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = format;
    clearValue.Color[0] = ClearColor.R();
    clearValue.Color[1] = ClearColor.G();
    clearValue.Color[2] = ClearColor.B();
    clearValue.Color[3] = ClearColor.A();

    CreateTextureResource(RenderDevice::Get().GetD3DDevice(), name, resourceDesc, clearValue, vidMemPtr);
    CreateDerivedViews(RenderDevice::Get().GetD3DDevice(), format, arrayCount, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::GenerateMipMaps(CommandContext& baseContext)
{
    if (numMipMaps == 0)
    {
        return;
    }

    ComputeContext& Context = baseContext.GetComputeContext();
    Context.SetRootSignature(RenderDevice::Get().GetGenerateMipsRootSig());
    Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.SetDynamicDescriptor(1, 0, SRVHandle);

    for (uint32_t topMip = 0; topMip < numMipMaps;)
    {
        uint32_t srcWidth  = Width >> topMip;
        uint32_t srcHeight = Height >> topMip;
        int32_t  dstWidth  = srcWidth >> 1;
        uint32_t dstHeight = srcHeight >> 1;

        // Determine if the first downsample is more than 2:1.  This happens whenever
        // the source width or height is odd.
        uint32_t nonPowerOfTwo = (srcWidth & 1) | (srcHeight & 1) << 1;
        if (Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        {
            Context.SetPipelineState(RenderDevice::Get().GetGenerateMipsGammaPSO()[nonPowerOfTwo]);
        }
        else
        {
            Context.SetPipelineState(RenderDevice::Get().GetGenerateMipsLinearPSO()[nonPowerOfTwo]);
        }

        // We can downsample up to four times, but if the ratio between levels is not
        // exactly 2:1, we have to shift our blend weights, which gets complicated or
        // expensive.  Maybe we can update the code later to compute sample weights for
        // each successive downsample.  We use _BitScanForward to count number of zeros
        // in the low bits.  Zeros indicate we can divide by two without truncating.
        uint32_t additionalMips;
        _BitScanForward((unsigned long*)&additionalMips, (dstWidth == 1 ? dstHeight : dstWidth) | (dstHeight == 1 ? dstWidth : dstHeight));

        uint32_t numMips = 1 + (additionalMips > 3 ? 3 : additionalMips);
        if (topMip + numMips > numMipMaps)
        {
            numMips = numMipMaps - topMip;
        }

        // These are clamped to 1 after computing additional mips because clamped
        // dimensions should not limit us from downsampling multiple times.  (E.g.
        // 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
        if (dstWidth == 0)
        {
            dstWidth = 1;
        }
        if (dstHeight == 0)
        {
            dstHeight = 1;
        }

        Context.SetConstants(0, topMip, numMips, 1.0f / dstWidth, 1.0f / dstHeight);
        Context.SetDynamicDescriptors(2, 0, numMips, UAVHandle + topMip + 1);
        Context.Dispatch2D(dstWidth, dstHeight);
        Context.InsertUAVBarrier(*this);

        topMip += numMips;
    }

    Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::SetClearColor(Color clearColor)
{
    ClearColor = clearColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ColorBuffer::SetMsaaMode(uint32_t numColorSamples, uint32_t numCoverageSamples)
{
    ASSERT(numCoverageSamples >= numColorSamples);
    FragmentCount = numColorSamples;
    SampleCount   = numCoverageSamples;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_RESOURCE_FLAGS ColorBuffer::CombineResourceFlags() const
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (flags == D3D12_RESOURCE_FLAG_NONE && FragmentCount == 1)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}
