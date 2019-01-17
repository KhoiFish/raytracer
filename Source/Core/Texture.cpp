#include "Texture.h"

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 ConstantTexture::Value(float u, float v, const Vec3& p) const
{
    return Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 CheckerTexture::Value(float u, float v, const Vec3& p) const
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

Vec3 NoiseTexture::Value(float u, float v, const Vec3& p) const
{
    return Vec3(1, 1, 1) * 0.5f * (1 + sin(Scale * p.Z() + 10 * Perlin::Turb(p)));
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 ImageTexture::Value(float u, float v, const Vec3& p) const
{
    int i = int((u)* Width);
    int j = int((1 - v) * Height);

    // Clamp
    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i > Width - 1)  i = Width - 1;
    if (j > Height - 1) j = Height - 1;

    int offset = 3 * i + 3 * Width * j;
    float r = int(Data[offset + 0]) / 255.f;
    float g = int(Data[offset + 1]) / 255.f;
    float b = int(Data[offset + 2]) / 255.f;

    return Vec3(r, g, b);
}
