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

#include "Vec4.h"
#include "Perlin.h"
#include <string>

// ----------------------------------------------------------------------------------------------------------------------------

class BaseTexture
{
public:

    virtual Vec4 Value(float u, float v, const Vec4& p) const = 0;
};

// ----------------------------------------------------------------------------------------------------------------------------

class ConstantTexture : public BaseTexture
{
public:

    inline ConstantTexture() {}
    inline ConstantTexture(const Vec4& color) : Color(color) {}

    virtual Vec4 Value(float u, float v, const Vec4& p) const;

private:

    Vec4 Color;
};

// ----------------------------------------------------------------------------------------------------------------------------

class CheckerTexture : public BaseTexture
{
public:

    inline CheckerTexture() {}
    inline CheckerTexture(BaseTexture* t0, BaseTexture* t1) : Odd(t0), Even(t1) {}

    virtual Vec4 Value(float u, float v, const Vec4& p) const;

private:

    BaseTexture* Odd;
    BaseTexture* Even;
};

// ----------------------------------------------------------------------------------------------------------------------------

class NoiseTexture : public BaseTexture
{
public:

    inline NoiseTexture(float scale) : Scale(scale) {}

    virtual Vec4 Value(float u, float v, const Vec4& p) const;

private:

    float Scale;
};

// ----------------------------------------------------------------------------------------------------------------------------

class ImageTexture : public BaseTexture
{
public:

    ImageTexture (const unsigned char* pixels, bool hasAlpha, int width, int height);
    ImageTexture(const char* filePath);
    ~ImageTexture();

    virtual Vec4   Value(float u, float v, const Vec4& p) const;

    std::string    GetSourceFilename() const { return Filename; }
    const Vec4*    GetImageFP() const        { return ImageData; }
    const uint8_t* GetImageRgba8888() const  { return ImageRgba; }

private:

    void createFromPixelData(const unsigned char* pixels, bool hasAlpha, int width, int height);

private:

    std::string     Filename;
    Vec4*           ImageData;
    uint8_t*        ImageRgba;
    int             Width, Height;
};