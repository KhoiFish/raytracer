#pragma once
#include "Vec3.h"
#include "Systems.h"
#include <vector>
#include <string>

// ----------------------------------------------------------------------------------------------------------------------------

#define RT_PI 3.14159265358979f

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

inline float CompareFloatEqual(float a, float b, float relTol = 0.0000001f, float absTol = 0.0000001f)
{
    return (fabs(a - b) <= GetMax<float>(absTol, relTol * GetMax<float>(fabs(a), fabs(b))));
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

inline float atan2Fast(float y, float x)
{
    //http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
    //Volkan SALMA

    const float ONEQTR_PI = RT_PI / 4.0f;
    const float THRQTR_PI = 3.0f * RT_PI / 4.0f;
    float r, angle;
    float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
    if (x < 0.0f)
    {
        r = (x + abs_y) / (abs_y - x);
        angle = THRQTR_PI;
    }
    else
    {
        r = (x - abs_y) / (x + abs_y);
        angle = ONEQTR_PI;
    }

    angle += (0.1963f * r * r - 0.9817f) * r;
    if (y < 0.0f)
    {
        return(-angle);     // negate if in quad III or IV
    }
    else
    {
        return(angle);
    }
}

inline float asinFast(float x)
{
    float negate = float(x < 0);
    x = abs(x);
    float ret = -0.0187293f;
    ret *= x;
    ret += 0.0742610f;
    ret *= x;
    ret -= 0.2121144f;
    ret *= x;
    ret += 1.5707288f;
    ret = 3.14159265358979f*0.5f - sqrt(1.0f - x)*ret;
    return ret - 2 * negate * ret;
}

inline void GetSphereUV(const Vec3& p, float& u, float& v)
{
    const float oneOverPi    = 1.f / (RT_PI);
    const float oneOverTwoPi = 1.f / (2 * RT_PI);

    const float phi   = atan2Fast(p.Z(), p.X());
    const float theta = asinFast(p.Y());

    u = 1.f - (phi + RT_PI) * oneOverTwoPi;
    v = (theta + RT_PI / 2) * oneOverPi;
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

// ----------------------------------------------------------------------------------------------------------------------------

inline std::vector<std::string> GetStringTokens(std::string sourceStr, std::string delim)
{
    std::vector<std::string> tokenList;
    size_t last = 0, next = 0;
    while ((next = sourceStr.find(delim, last)) != std::string::npos)
    {
        tokenList.push_back(sourceStr.substr(last, next - last + 1));
        last = next + delim.length();
    }
    tokenList.push_back(sourceStr.substr(last, next - last + 1));

    return tokenList;
}
