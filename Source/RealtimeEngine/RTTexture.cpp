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

#include "RTTexture.h"
#include "RenderDevice.h"
#include "CommandContext.h"
#include "StbImage/stb_image.h"
#include <map>
#include <mutex>
#include <thread>

using namespace std;
using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

static string_t                                    sRootPath = "";
static map< string_t, unique_ptr<ManagedTexture> > sTextureCache;

// ----------------------------------------------------------------------------------------------------------------------------

static inline size_t BitsPerPixel(_In_ DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

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
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

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
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

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
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline uint32_t BytesPerPixel(DXGI_FORMAT format)
{
    return (uint32_t)BitsPerPixel(format) / 8;
};

// ----------------------------------------------------------------------------------------------------------------------------

void Texture::Create(size_t pitch, size_t width, size_t height, DXGI_FORMAT format, const void* initialData)
{
    UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width              = width;
    texDesc.Height             = (uint32_t)height;
    texDesc.DepthOrArraySize   = 1;
    texDesc.MipLevels          = 1;
    texDesc.Format             = format;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        UsageState, nullptr, IID_PPV_ARGS(ResourcePtr.ReleaseAndGetAddressOf())));

    ResourcePtr->SetName(L"Texture");

    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData      = initialData;
    texResource.RowPitch   = pitch * BytesPerPixel(format);
    texResource.SlicePitch = texResource.RowPitch * height;

    CommandContext::InitializeTexture(*this, 1, &texResource);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::Texture::Create(size_t width, size_t height, DXGI_FORMAT format, const void* initData)
{
    Create(width, width, height, format, initData);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::Texture::Destroy()
{
    GpuResource::Destroy();
}

// ----------------------------------------------------------------------------------------------------------------------------

void ManagedTexture::WaitForLoad() const
{
    /*volatile D3D12_CPU_DESCRIPTOR_HANDLE&  volHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)CpuDescriptorHandle;
    volatile bool&                         volValid  = (volatile bool&)IsValidState;
    while (volHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && volValid)
    {
        this_thread::yield();
    }*/

    return;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RealtimeEngine::ManagedTexture::IsValid() const
{
    return IsValidState;
}

// ----------------------------------------------------------------------------------------------------------------------------

void TextureManager::Initialize(const string_t& TextureLibRoot)
{
    sRootPath = TextureLibRoot;
}

// ----------------------------------------------------------------------------------------------------------------------------

void TextureManager::Shutdown(void)
{
    sTextureCache.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline pair<ManagedTexture*, bool> FindOrLoadTexture(const string_t& fileName)
{
    static mutex sMutex;
    lock_guard<mutex> guard(sMutex);

    auto iter = sTextureCache.find(fileName);

    // If it's found, it has already been loaded or the load process has begun
    if (iter != sTextureCache.end())
    {
        return make_pair(iter->second.get(), false);
    }

    ManagedTexture * newTexture = new ManagedTexture(fileName);
    sTextureCache[fileName].reset(newTexture);

    // This was the first time it was requested, so indicate that the caller must read the file
    return make_pair(newTexture, true);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline uint32_t rgbaToHex(float r, float g, float b, float a)
{
    // Clamp
    r = std::min(r, 1.0f);
    g = std::min(g, 1.0f);
    b = std::min(b, 1.0f);
    a = std::min(a, 1.0f);

    uint8_t cR = uint8_t(r * 255.0f);
    uint8_t cG = uint8_t(g * 255.0f);
    uint8_t cB = uint8_t(b * 255.0f);
    uint8_t cA = uint8_t(a * 255.0f);

    return ((cA << 24) | (cB << 16) | (cG << 8) | (cR << 0));
}

// ----------------------------------------------------------------------------------------------------------------------------

Texture& TextureManager::CreateFromColor(float r, float g, float b, float a)
{
    char hashName[256];
    sprintf_s(hashName, "%f %f %f", r, g, b);

    auto            managedTex   = FindOrLoadTexture(hashName);
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;
    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return *manTex;
    }

    uint32_t pixelColor = rgbaToHex(r, g, b, a);
    manTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &pixelColor);

    return *manTex;
}

// ----------------------------------------------------------------------------------------------------------------------------

Texture& TextureManager::GetBlackTex2D()
{
    auto            managedTex   = FindOrLoadTexture("DefaultBlackTexture");
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;
    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return *manTex;
    }

    uint32_t blackPixel = 0;
    manTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &blackPixel);

    return *manTex;
}

// ----------------------------------------------------------------------------------------------------------------------------

Texture& TextureManager::GetWhiteTex2D()
{
    auto            managedTex   = FindOrLoadTexture("DefaultWhiteTexture");
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;

    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return *manTex;
    }

    uint32_t whitePixel = 0xFFFFFFFFul;
    manTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &whitePixel);

    return *manTex;
}

// ----------------------------------------------------------------------------------------------------------------------------

Texture& TextureManager::GetMagentaTex2D()
{
    auto            managedTex   = FindOrLoadTexture("DefaultMagentaTexture");
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;

    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return *manTex;
    }

    uint32_t MagentaPixel = 0x00FF00FF;
    manTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);

    return *manTex;
}

// ----------------------------------------------------------------------------------------------------------------------------

ManagedTexture* TextureManager::LoadFromFile(const string_t & fileName, bool sRGB)
{
    auto            managedTex   = FindOrLoadTexture(fileName);
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;
    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return manTex;
    }

    int comp;
    unsigned char* pixelData = stbi_load(fileName.c_str(), &manTex->Width, &manTex->Height, &comp, STBI_rgb_alpha);
    if (pixelData != nullptr)
    {
        manTex->Create(manTex->Width, manTex->Height, sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, pixelData);
        manTex->GetResource()->SetName(MakeWStr(fileName).c_str());
        free(pixelData);
        pixelData = nullptr;
    }
    else
    {
        DEBUG_PRINTF("%s\n", stbi_failure_reason());
    }

    return manTex;
}
