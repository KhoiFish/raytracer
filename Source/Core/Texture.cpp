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
#include <StbImage/stb_image.h>

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 ConstantTexture::Value(float u, float v, const Vec4& p) const
{
    return Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 CheckerTexture::Value(float u, float v, const Vec4& p) const
{
    float sines = sin(10.f * p.X()) * sin(10.f * p.Y()) * sin(10.f * p.Z());
    if (sines < 0)
    {
        return Odd->Value(u, v, p);
    }
    else
    {
        return Even->Value(u, v, p);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 NoiseTexture::Value(float u, float v, const Vec4& p) const
{
    return Vec4(1, 1, 1) * 0.5f * (1 + sin(Scale * p.Z() + 10 * Perlin::Turb(p)));
}

// ----------------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const unsigned char* pixels, bool hasAlpha, int width, int height) : Width(width), Height(height)
{
    createFromPixelData(pixels, hasAlpha, width, height);
}

// ----------------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const char* filePath) : Filename(filePath)
{
    int comp;
    unsigned char* pixelData = stbi_load(filePath, &Width, &Height, &comp, STBI_rgb_alpha);
    if (pixelData != nullptr)
    {
        createFromPixelData(pixelData, true, Width, Height);
        free(pixelData);
        pixelData = nullptr;
    }
    else
    {
        DEBUG_PRINTF("%s\n", stbi_failure_reason());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ImageTexture::~ImageTexture()
{
    if (ImageData != nullptr)
    {
        delete [] ImageData;
        ImageData = nullptr;
    }
    
    if (ImageRgba != nullptr)
    {
        delete ImageRgba;
        ImageRgba = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 ImageTexture::Value(float u, float v, const Vec4& p) const
{
    int i = int((u)* Width);
    int j = int((1 - v) * Height);

    // Clamp
    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i > Width - 1)  i = Width - 1;
    if (j > Height - 1) j = Height - 1;

    const int offset = (i) + (Width * j);

    return ImageData[offset];
}

// ----------------------------------------------------------------------------------------------------------------------------

void ImageTexture::createFromPixelData(const unsigned char* pixels, bool hasAlpha, int width, int height)
{
    ImageData = new Vec4[width * height];
    ImageRgba = new uint8_t[width * height * 4];

    const int bpp = hasAlpha ? 4 : 3;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const int   srcOffset = (bpp * x) + (bpp * Width * y);
            const int   dstOffset = (x)+(Width * y);
            const float kRemap    = 1.f / 255.f;

            const float r = int(pixels[srcOffset + 0]) * kRemap;
            const float g = int(pixels[srcOffset + 1]) * kRemap;
            const float b = int(pixels[srcOffset + 2]) * kRemap;

            // Has alpha?
            const float a = hasAlpha ? (int(pixels[srcOffset + 3]) * kRemap) : 1.f;

            ImageRgba[srcOffset + 0] = pixels[srcOffset + 0];
            ImageRgba[srcOffset + 1] = pixels[srcOffset + 1];
            ImageRgba[srcOffset + 2] = pixels[srcOffset + 2];
            if (hasAlpha)
            {
                ImageRgba[srcOffset + 3] = pixels[srcOffset + 3];
            }
            else
            {
                ImageRgba[srcOffset + 3] = 255;
            }
                
            ImageData[dstOffset] = Vec4(r, g, b, a);
        }
    }
}
