#pragma once

#include "Vec3.h"
#include "Perlin.h"

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

    inline ImageTexture (unsigned char* pixels, int width, int height)
        : Data(pixels), Width(width), Height(height) {}

    virtual Vec3 Value(float u, float v, const Vec3& p) const;

private:

    unsigned char*  Data;
    int             Width, Height;
};