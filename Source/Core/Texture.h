#pragma once

#include "Vec3.h"
#include "Perlin.h"
#include <string>

// ----------------------------------------------------------------------------------------------------------------------------

class BaseTexture
{
public:

    virtual Vec3 Value(float u, float v, const Vec3& p) const = 0;
};

// ----------------------------------------------------------------------------------------------------------------------------

class ConstantTexture : public BaseTexture
{
public:

    inline ConstantTexture() {}
    inline ConstantTexture(Vec3 color) : Color(color) {}

    virtual Vec3 Value(float u, float v, const Vec3& p) const;

private:

    Vec3 Color;
};

// ----------------------------------------------------------------------------------------------------------------------------

class CheckerTexture : public BaseTexture
{
public:

    inline CheckerTexture() {}
    inline CheckerTexture(BaseTexture* t0, BaseTexture* t1) : Odd(t0), Even(t1) {}

    virtual Vec3 Value(float u, float v, const Vec3& p) const;

private:

    BaseTexture* Odd;
    BaseTexture* Even;
};

// ----------------------------------------------------------------------------------------------------------------------------

class NoiseTexture : public BaseTexture
{
public:

    inline NoiseTexture(float scale) : Scale(scale) {}

    virtual Vec3 Value(float u, float v, const Vec3& p) const;

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

    virtual Vec3   Value(float u, float v, const Vec3& p) const;

    std::string    GetSourceFilename() const { return Filename; }
    const Vec3*    GetImageFP() const        { return ImageData; }
    const uint8_t* GetImageRgba8888() const  { return ImageRgba; }

private:

    void createFromPixelData(const unsigned char* pixels, bool hasAlpha, int width, int height);

private:

    std::string     Filename;
    Vec3*           ImageData;
    uint8_t*        ImageRgba;
    int             Width, Height;
};