#pragma once

#include "Vec4.h"
#include "vcl/vector3d.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Ray
{
public:

    inline Ray() {}

    inline Ray(const Vec4& a, const Vec4& b, float ti = 0.f)
        : A(a), B(b), Timestamp(ti)
    {
        FastA    = Vec3f(A[0], A[1], A[2]);
        FastB    = Vec3f(B[0], B[1], B[2]);
        FastInvB = Vec3f(1.f) / FastB;
        
        for (int i = 0; i < 3; i++)
        {
            InvBArray[i] = FastInvB[i];
        }
    }

    inline Vec4  Origin() const                  { return A; }
    inline Vec4  Direction() const               { return B; }
    inline float Time() const                    { return Timestamp; }
    inline Vec4  PointAtParameter(float t) const { return A + (t * B); }

    inline Vec3f OriginFast() const                  { return FastA; }
    inline Vec3f DirectionFast() const               { return FastB; }
    inline Vec3f InverseDirectionFast() const        { return FastInvB; }
    inline Vec3f PointAtParameterFast(float t) const { return FastA + (t * FastB); }

    inline const float* InverseDirectionArray() const { return InvBArray; }

private:

    Vec4   A;
    Vec4   B;
    float  Timestamp;

    Vec3f  FastA, FastB, FastInvB;
    float  InvBArray[3];
};