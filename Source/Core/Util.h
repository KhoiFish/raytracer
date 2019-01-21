#pragma once
#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define DEBUG_PRINTF(fmt, ...) RenderDebugPrintf(fmt, ##__VA_ARGS__)

void RenderDebugPrintf(const char *fmt, ...);

// ----------------------------------------------------------------------------------------------------------------------------

#define RT_PI 3.14159265359f

inline float DegreesToRadians(float degrees)
{
    return (degrees * RT_PI / 180.f);
}

inline float RadiansToDegrees(float radians)
{
    return (radians * 180.f / RT_PI);
}

// ----------------------------------------------------------------------------------------------------------------------------

void PrintProgress(const char* otherInfo, double percentage);

// ----------------------------------------------------------------------------------------------------------------------------

template<typename T>
constexpr const T& GetMax(const T& a, const T& b)
{
    return (a > b) ? a : b;
}

template<typename T>
constexpr const T& GetMin(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

template<typename T>
constexpr const T& Clamp(const T& val, const T& min, const T& max)
{
    return val < min ? min : val > max ? max : val;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float RandomFloat()
{
    float num = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    return num;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomInUnitSphere()
{
    Vec3 p;
    do
    {
        p = 2.0f * Vec3(RandomFloat(), RandomFloat(), RandomFloat()) - Vec3(1.f, 1.f, 1.f);
    } while (p.SquaredLength() >= 1.f);

    return p;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomInUnitDisk()
{
    Vec3 p;
    do 
    {
        p = 2.0f * Vec3(RandomFloat(), RandomFloat(), 0) - Vec3(1, 1, 0);
    } while (Dot(p, p) >= 1.0f);

    return p;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomToSphere(float radius, float distanceSquared)
{
    float r1  = RandomFloat();
    float r2  = RandomFloat();
    float  z  = 1 + r2 * (sqrt(1 - radius * radius / distanceSquared) - 1);
    float phi = 2 * RT_PI * r1;
    float x   = cos(phi) * sqrt(1 - z * z);
    float y   = sin(phi) * sqrt(1 - z * z);

    return Vec3(x, y, z);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomCosineDirection()
{
    float r1  = RandomFloat();
    float r2  = RandomFloat();
    float z   = sqrt(1 - r2);
    float phi = 2 * RT_PI * r1;
    float x   = cos(phi) * 2 * sqrt(r2);
    float y   = sin(phi) * 2 * sqrt(r2);

    return Vec3(x, y, z);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float Schlick(float cosine, float refIdx)
{
    float r0 = (1 - refIdx) / (1 + refIdx);
    r0 = r0 * r0;
    return r0 + (1 - r0)*pow(1 - cosine, 5);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float TrilinearInterp(float c[2][2][2], float u, float v, float w)
{
    float accum = 0;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                accum +=
                    (i*u + (1 - i)*(1 - u)) *
                    (j*v + (1 - j)*(1 - v)) *
                    (k*w + (1 - k)*(1 - w)) *
                    c[i][j][k];
            }
        }
    }

    return accum;
}

// ----------------------------------------------------------------------------------------------------------------------------

// This method produces incorrect output, as described in the github
inline float PerlinInterp(Vec3 c[2][2][2], float u, float v, float w)
{
    float uu = u * u * (3 - 2 * u);
    float vv = v * v * (3 - 2 * v);
    float ww = w * w * (3 - 2 * w);
    float accum = 0;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                Vec3 weight(u - i, v - j, w - k);
                accum +=
                    (i*uu + (1 - i)*(1 - uu)) *
                    (j*vv + (1 - j)*(1 - vv)) *
                    (k*ww + (1 - k)*(1 - ww)) *
                    Dot(c[i][j][k], weight);
            }
        }
    }

    return fabs(accum);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline void GetSphereUV(const Vec3& p, float& u, float& v)
{
    float phi   = atan2(p.Z(), p.X());
    float theta = asin(p.Y());
    u = 1 - (phi + RT_PI) / (2 * RT_PI);
    v = (theta + RT_PI / 2) / RT_PI;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline void GetRGBA8888(Vec3 col, bool gammaCorrect, int& ir, int& ig, int& ib, int& ia)
{
    // Gamma correct
    if (gammaCorrect)
    {
        col = Vec3(sqrt(col.R()), sqrt(col.G()), sqrt(col.B()));
    }

    // Write pixel to file
    ir = int(255.99*col.R());
    ig = int(255.99*col.G());
    ib = int(255.99*col.B());
    ia = 255;

    ir = (ir > 255) ? 255 : ir;
    ig = (ig > 255) ? 255 : ig;
    ib = (ib > 255) ? 255 : ib;
}
