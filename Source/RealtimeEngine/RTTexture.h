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
#include "GpuResource.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class Texture : public GpuResource
    {
    public:

        Texture() { CpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
        Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : CpuDescriptorHandle(Handle) {}

        void                                Create(size_t pitch, size_t width, size_t height, DXGI_FORMAT Format, const void* initData);
        void                                Create(size_t width, size_t height, DXGI_FORMAT format, const void* initData);
        void                                CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
        void                                CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize);
        virtual void                        Destroy() override;
        const D3D12_CPU_DESCRIPTOR_HANDLE&  GetSRV() const;

        bool operator!() { return CpuDescriptorHandle.ptr == 0; }

    protected:

        D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ManagedTexture : public Texture
    {
    public:

        ManagedTexture(const string_t& fileName) : MapKey(fileName), IsValidState(true) {}

        void WaitForLoad() const;
        void Unload();
        void SetToInvalidTexture();
        bool IsValid() const;
        void operator= (const Texture& Texture);

    private:

        string_t  MapKey;
        bool      IsValidState;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class TextureManager
    {
    public:

        static void                   Initialize(const string_t& textureLibRoot);
        static void                   Shutdown();
        static const ManagedTexture*  LoadFromFile(const string_t& fileName, bool sRGB = false);
        static const ManagedTexture*  LoadTGAFromFile(const string_t& fileName, bool sRGB = false);
        static const ManagedTexture*  LoadPIXImageFromFile(const string_t& fileName);
        static const Texture&         GetBlackTex2D();
        static const Texture&         GetWhiteTex2D();
        static const Texture&         GetMagentaTex2D();
    };
}
