#pragma once
#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define RT_PI 3.14159265359f

// ----------------------------------------------------------------------------------------------------------------------------

void PrintProgress(const char* otherInfo, double percentage);

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
