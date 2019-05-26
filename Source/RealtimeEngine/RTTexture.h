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
#include "DescriptorHeapStack.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class Texture : public GpuResource
    {
    public:

        Texture() : Width(0), Height(0), Format(DXGI_FORMAT_UNKNOWN) {}

        void            Create(size_t pitch, size_t width, size_t height, DXGI_FORMAT Format, const void* initData);
        void            Create(size_t width, size_t height, DXGI_FORMAT format, const void* initData);
        virtual void    Destroy() override;
        DXGI_FORMAT     GetFormat() const { return Format; }

    protected:

        friend class TextureManager;

        int32_t         Width;
        int32_t         Height;
        DXGI_FORMAT     Format;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ManagedTexture : public Texture
    {
    public:

        ManagedTexture(const string_t& fileName) : MapKey(fileName), IsValidState(true) {}

        void WaitForLoad() const;
        void Unload();
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

        static void             Initialize(const string_t& textureLibRoot);
        static void             Shutdown();
        static ManagedTexture*  LoadFromFile(const string_t& fileName, bool sRGB = false);
        static Texture&         CreateFromColor(float r, float g, float b, float a);
        static Texture&         GetBlackTex2D();
        static Texture&         GetWhiteTex2D();
        static Texture&         GetMagentaTex2D();
    };
}
