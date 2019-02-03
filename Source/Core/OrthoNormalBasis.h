#pragma once

#include "Vec4.h"

// ----------------------------------------------------------------------------------------------------------------------------

class OrthoNormalBasis
{
public:

    inline Vec4 operator[] (int i) const { return Axis[i]; }

    inline Vec4 U() const { return Axis[0]; }
    inline Vec4 V() const { return Axis[1]; }
    inline Vec4 W() const { return Axis[2]; }

    inline Vec4 Local(float a, float b, float c) const { return a * U() + b * V() + c * W(); }
    inline Vec4 Local(const Vec4& v) const { return v.X() * U() + v.Y() * V() + v.Z() * W(); }
    
    inline void BuildFromW(const Vec4& n)
    {
        Axis[2] = UnitVector(n);

        Vec4 a;
        if (fabs(W().X()) > 0.9f)
        {
            a = Vec4(0, 1, 0);
        }
        else
        {
            a = Vec4(1, 0, 0);
        }

        Axis[1] = UnitVector(Cross(W(), a));
        Axis[0] = Cross(W(), V());
    }

private:

    Vec4 Axis[3];
};
