#pragma once
#include "Ray.h"
#include "Vec4.h"
#include "Util.h"
#include <vcl/vector3d.h>

// ----------------------------------------------------------------------------------------------------------------------------

class AABB
{
public:

    inline AABB() {}
    inline AABB(const Vec4& minP, const Vec4& maxP) : MinP(minP), MaxP(maxP)
    {
        FastMinP = Vec3f(minP[0], minP[1], minP[2]);
        FastMaxP = Vec3f(maxP[0], maxP[1], maxP[2]);
    }

    inline Vec4 Min() const { return MinP; }
    inline Vec4 Max() const { return MaxP; }

    inline bool Hit(const Ray& ray, float tMin, float tMax) const
    {
        // Do the math fast using SIMD
        Vec3f vT0 = (FastMinP - ray.OriginFast()) * ray.InverseDirectionFast();
        Vec3f vT1 = (FastMaxP - ray.OriginFast()) * ray.InverseDirectionFast();

        // Extract the data from the SIMD registers
        FloatAligned16 t0, t1;
        static_cast<Vec4f>(vT0).store_a(t0.Data);
        static_cast<Vec4f>(vT1).store_a(t1.Data);

        // Run through the cases and early reject
        for (int i = 0; i < 3; i++)
        {
            float data[2]  = { t0.Data[i], t1.Data[i] };
            int   minIndex = (ray.InverseDirectionArray()[i] < 0.f) ? 1 : 0;
            int   maxIndex = (minIndex + 1) % 2;

            tMin = data[minIndex] > tMin ? data[minIndex] : tMin;
            tMax = data[maxIndex] < tMax ? data[maxIndex] : tMax;
            if (tMax < tMin)
            {
                return false;
            }
        }

        return true;
    }

    static inline AABB ComputerAABBForSphere(const Vec4& center, float radius)
    {
        Vec4 temp(radius, radius, radius);
        return AABB(center - temp, center + temp);
    }

    static inline AABB SurroundingBox(AABB box0, AABB box1)
    {
        Vec4 small(
            fmin(box0.MinP.X(), box1.MinP.X()),
            fmin(box0.MinP.Y(), box1.MinP.Y()),
            fmin(box0.MinP.Z(), box1.MinP.Z())
        );

        Vec4 big(
            fmax(box0.Max().X(), box1.Max().X()),
            fmax(box0.Max().Y(), box1.Max().Y()),
            fmax(box0.Max().Z(), box1.Max().Z())
        );

        return AABB(small, big);
    }

private:

    #pragma pack(push, 16)
        struct FloatAligned16
        {
            float Data[4];
        };
    #pragma pack(pop)

private:

    Vec4   MinP;
    Vec4   MaxP;
    Vec3f  FastMinP;
    Vec3f  FastMaxP;
};
