#pragma once
#include "Ray.h"
#include "Vec4.h"
#include "Util.h"

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
        const Vec4f vT0  = static_cast<Vec4f>((FastMinP - ray.OriginFast()) * ray.InverseDirectionFast());
        const Vec4f vT1  = static_cast<Vec4f>((FastMaxP - ray.OriginFast()) * ray.InverseDirectionFast());
        const Vec4f vMin = min(vT0, vT1);
        const Vec4f vMax = max(vT0, vT1);

        // Extract the data from the SIMD registers
        FloatAligned16 minData, maxData;
        vMin.store_a(minData.Data);
        vMax.store_a(maxData.Data);

        // Horizontal min/max
        const float minVal = GetMax(minData.Data[2], GetMax(minData.Data[0], minData.Data[1]));
        const float maxVal = GetMin(maxData.Data[2], GetMin(maxData.Data[0], maxData.Data[1]));

        return (maxVal >= GetMax(tMin, minVal) && minVal < tMax);
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
