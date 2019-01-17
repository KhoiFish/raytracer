#pragma once

#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Ray
{
public:

    inline Ray() {}

    inline Ray(const Vec3& a, const Vec3& b, float ti = 0.f)
        : A(a), B(b), Timestamp(ti)
    {}

    inline Vec3  Origin() const    { return A; }
    inline Vec3  Direction() const { return B; }
    inline float Time() const      { return Timestamp; }

    inline Vec3 PointAtParameter(float t) const
    {
        return A + (t * B);
    }

private:

    Vec3   A;
    Vec3   B;
    float  Timestamp;
};