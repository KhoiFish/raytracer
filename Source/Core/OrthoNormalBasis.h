#pragma once

#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class OrthoNormalBasis
{
public:

    inline Vec3 operator[] (int i) const { return Axis[i]; }

    inline Vec3 U() const { return Axis[0]; }
    inline Vec3 V() const { return Axis[1]; }
    inline Vec3 W() const { return Axis[2]; }

    inline Vec3 Local(float a, float b, float c) const { return a * U() + b * V() + c * W(); }
    inline Vec3 Local(const Vec3& v) const { return v.X() * U() + v.Y() * V() + v.Z() * W(); }
    
    inline void BuildFromW(const Vec3& n)
    {
        Axis[2] = UnitVector(n);

        Vec3 a;
        if (fabs(W().X()) > 0.9f)
        {
            a = Vec3(0, 1, 0);
        }
        else
        {
            a = Vec3(1, 0, 0);
        }

        Axis[1] = UnitVector(Cross(W(), a));
        Axis[0] = Cross(W(), V());
    }

private:

    Vec3 Axis[3];
};
