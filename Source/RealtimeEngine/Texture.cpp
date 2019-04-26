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

#include "Texture.h"
#include "RenderDevice.h"
#include "CommandContext.h"
#include <map>
#include <mutex>
#include <thread>

using namespace std;
using namespace yart;

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

static inline UINT BytesPerPixel(DXGI_FORMAT Format)
{
    return (UINT)BitsPerPixel(Format) / 8;
};

// ----------------------------------------------------------------------------------------------------------------------------

void Texture::Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData)
{
    UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width              = Width;
    texDesc.Height             = (UINT)Height;
    texDesc.DepthOrArraySize   = 1;
    texDesc.MipLevels          = 1;
    texDesc.Format             = Format;
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

    ASSERT_SUCCEEDED(RenderDevice::Get()->GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        UsageState, nullptr, IID_PPV_ARGS(ResourcePtr.ReleaseAndGetAddressOf())));

    ResourcePtr->SetName(L"Texture");

    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData      = InitialData;
    texResource.RowPitch   = Pitch * BytesPerPixel(Format);
    texResource.SlicePitch = texResource.RowPitch * Height;

    CommandContext::InitializeTexture(*this, 1, &texResource);

    if (CpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        CpuDescriptorHandle = RenderDevice::Get()->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    RenderDevice::Get()->GetD3DDevice()->CreateShaderResourceView(ResourcePtr.Get(), nullptr, CpuDescriptorHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void yart::Texture::Create(size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData)
{
    Create(Width, Width, Height, Format, InitData);
}

// ----------------------------------------------------------------------------------------------------------------------------

void yart::Texture::Destroy()
{
    GpuResource::Destroy();

    // This leaks descriptor handles.  We should really give it back to be reused.
    CpuDescriptorHandle.ptr = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

const D3D12_CPU_DESCRIPTOR_HANDLE& yart::Texture::GetSRV() const
{
    return CpuDescriptorHandle;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Texture::CreateTGAFromMemory(const void* _filePtr, size_t, bool sRGB)
{
    const uint8_t* filePtr = (const uint8_t*)_filePtr;

    // Skip first two bytes
    filePtr += 2;

    /*uint8_t imageTypeCode =*/ *filePtr++;

    // Ignore another 9 bytes
    filePtr += 9;

    uint16_t imageWidth = *(uint16_t*)filePtr;
    filePtr += sizeof(uint16_t);
    uint16_t imageHeight = *(uint16_t*)filePtr;
    filePtr += sizeof(uint16_t);
    uint8_t bitCount = *filePtr++;

    // Ignore another byte
    filePtr++;

    uint32_t* formattedData = new uint32_t[imageWidth * imageHeight];
    uint32_t* iter = formattedData;

    uint8_t numChannels = bitCount / 8;
    uint32_t numBytes = imageWidth * imageHeight * numChannels;

    switch (numChannels)
    {
    default:
        break;

    case 3:
        for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 3)
        {
            *iter++ = 0xff000000 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
            filePtr += 3;
        }
        break;

    case 4:
        for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 4)
        {
            *iter++ = filePtr[3] << 24 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
            filePtr += 4;
        }
        break;
    }

    Create(imageWidth, imageHeight, sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, formattedData);

    delete[] formattedData;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Texture::CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize)
{
    struct Header
    {
        DXGI_FORMAT Format;
        uint32_t Pitch;
        uint32_t Width;
        uint32_t Height;
    };
    const Header& header = *(Header*)memBuffer;

    ASSERT(fileSize >= header.Pitch * BytesPerPixel(header.Format) * header.Height + sizeof(Header),
        "Raw PIX image dump has an invalid file size");

    Create(header.Pitch, header.Width, header.Height, header.Format, (uint8_t*)memBuffer + sizeof(Header));
}

// ----------------------------------------------------------------------------------------------------------------------------

void ManagedTexture::WaitForLoad() const
{
    volatile D3D12_CPU_DESCRIPTOR_HANDLE&  volHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)CpuDescriptorHandle;
    volatile bool&                         volValid  = (volatile bool&)IsValidState;
    while (volHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && volValid)
    {
        this_thread::yield();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void ManagedTexture::SetToInvalidTexture()
{
    CpuDescriptorHandle = TextureManager::GetMagentaTex2D().GetSRV();
    IsValidState = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool yart::ManagedTexture::IsValid() const
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

const Texture& TextureManager::GetBlackTex2D()
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

const Texture& TextureManager::GetWhiteTex2D()
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

const Texture& TextureManager::GetMagentaTex2D()
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

const ManagedTexture* TextureManager::LoadFromFile(const string_t & fileName, bool sRGB)
{
    string_t              catPath = fileName;
    const ManagedTexture* tex     = LoadTGAFromFile(catPath + ".tga", sRGB);

    return tex;
}

// ----------------------------------------------------------------------------------------------------------------------------

const ManagedTexture * TextureManager::LoadTGAFromFile(const string_t & fileName, bool sRGB)
{
#if 0
    auto            managedTex   = FindOrLoadTexture(fileName);
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;
    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return manTex;
    }

    Utility::ByteArray ba = Utility::ReadFileSync(sRootPath + fileName);
    if (ba->size() > 0)
    {
        manTex->CreateTGAFromMemory(ba->data(), ba->size(), sRGB);
        manTex->GetResource()->SetName(MakeWStr(fileName).c_str());
    }
    else
    {
        manTex->SetToInvalidTexture();
    }

    return manTex;
#endif

    assert(false);
    return nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

const ManagedTexture* TextureManager::LoadPIXImageFromFile(const string_t & fileName)
{
#if 0
    auto            managedTex   = FindOrLoadTexture(fileName);
    ManagedTexture* manTex       = managedTex.first;
    const bool      requestsLoad = managedTex.second;

    if (!requestsLoad)
    {
        manTex->WaitForLoad();
        return manTex;
    }

    Utility::ByteArray ba = Utility::ReadFileSync(sRootPath + fileName);
    if (ba->size() > 0)
    {
        manTex->CreatePIXImageFromMemory(ba->data(), ba->size());
        manTex->GetResource()->SetName(MakeWStr(fileName).c_str());
    }
    else
    {
        manTex->SetToInvalidTexture();
    }

    return manTex;
#endif

    assert(false);
    return nullptr;
}
