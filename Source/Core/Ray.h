#pragma once

#include "Vec4.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Ray
{
public:

    inline Ray() {}

    inline Ray(const Vec4& a, const Vec4& b, float ti = 0.f)
        : A(a), B(b), Timestamp(ti)
    {}

    inline Vec4  Origin() const    { return A; }
    inline Vec4  Direction() const { return B; }
    inline float Time() const      { return Timestamp; }

    inline Vec4 PointAtParameter(float t) const
    {
        return A + (t * B);
    }

private:

    Vec4   A;
    Vec4   B;
    float  Timestamp;
};