// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <math.h>
#include <stdlib.h>
#include <iostream>
#include "Systems.h"

// ----------------------------------------------------------------------------------------------------------------------------

#if defined(ENABLE_VEC3_SANITY_CHECK)
    #define VEC3_SANITY_CHECK(vec) vec.SanityCheck()
#else
    #define VEC3_SANITY_CHECK(vec)
#endif

// ----------------------------------------------------------------------------------------------------------------------------

class Vec4
{
public:

    inline Vec4() {}

    inline Vec4(float e0, float e1, float e2, float e3 = 1.f)
    {
        e[0] = e0; e[1] = e1; e[2] = e2; e[3] = e3;
    }

    inline float X() const { return e[0]; }
    inline float Y() const { return e[1]; }
    inline float Z() const { return e[2]; }
    inline float W() const { return e[3]; }

    inline float R() const { return e[0]; }
    inline float G() const { return e[1]; }
    inline float B() const { return e[2]; }
    inline float A() const { return e[3]; }

    inline float& X() { return e[0]; }
    inline float& Y() { return e[1]; }
    inline float& Z() { return e[2]; }
    inline float& W() { return e[3]; }

    inline float& R() { return e[0]; }
    inline float& G() { return e[1]; }
    inline float& B() { return e[2]; }
    inline float& A() { return e[3]; }

    inline const Vec4& operator+() const       { return *this; }
    inline Vec4        operator-() const       { return Vec4(-e[0], -e[1], -e[2]); }
    inline float       operator[](int i) const { return e[i]; }
    inline float&      operator[](int i)       { return e[i]; }

    inline Vec4& operator+=(const Vec4 &v2);
    inline Vec4& operator-=(const Vec4 &v2);
    inline Vec4& operator*=(const Vec4 &v2);
    inline Vec4& operator/=(const Vec4 &v2);
    inline Vec4& operator*=(const float t);
    inline Vec4& operator/=(const float t);

    inline float Length() const
    {
        const float length = sqrt(SquaredLength());
        SANITY_CHECK_FLOAT(length);
        return length;
    }

    inline float SquaredLength() const
    {
        const float sqrLength = (e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
        SANITY_CHECK_FLOAT(sqrLength);
        return sqrLength;
    }

    inline void Clamp(float minVal, float maxVal)
    {
        for (int i = 0; i < 3; i++)
        {
            e[i] = e[i] < minVal ? minVal : e[i] > maxVal ? maxVal : e[i];
        }
    }

    inline void SanityCheck() const
    {
        SANITY_CHECK_FLOAT(e[0]);
        SANITY_CHECK_FLOAT(e[1]);
        SANITY_CHECK_FLOAT(e[2]);
    }

    inline Vec4 MakeUnitVector();

private:

    float e[4];
};

// ----------------------------------------------------------------------------------------------------------------------------

inline std::istream& operator>>(std::istream &is, Vec4 &t)
{
    is >> t.X() >> t.Y() >> t.Z();
    return is;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream &os, const Vec4 &t)
{
    os << t.X() << " " << t.Y() << " " << t.Z();
    return os;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 Vec4::MakeUnitVector()
{
    float k = 1.0f / Length();
    X() *= k;
    Y() *= k;
    Z() *= k;

    VEC3_SANITY_CHECK((*this));

    return (*this);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator+(const Vec4 &v1, const Vec4 &v2)
{
    Vec4 ret(
        v1.X() + v2.X(),
        v1.Y() + v2.Y(),
        v1.Z() + v2.Z());

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator-(const Vec4 &v1, const Vec4 &v2)
{
    Vec4 ret(
        v1.X() - v2.X(),
        v1.Y() - v2.Y(),
        v1.Z() - v2.Z());

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator*(const Vec4 &v1, const Vec4 &v2)
{
    Vec4 ret(
        v1.X() * v2.X(),
        v1.Y() * v2.Y(),
        v1.Z() * v2.Z());

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator/(const Vec4 &v1, const Vec4 &v2)
{
    Vec4 ret(
        v1.X() / v2.X(),
        v1.Y() / v2.Y(),
        v1.Z() / v2.Z());

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator*(float t, const Vec4 &v)
{
    Vec4 ret(
        t * v.X(),
        t * v.Y(),
        t * v.Z());

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator*(const Vec4 &v, float t)
{
    Vec4 ret = t * v;

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 operator/(const Vec4 &v, float t)
{
    Vec4 ret(
        v.X() / t,
        v.Y() / t,
        v.Z() / t);

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float Dot(const Vec4 &v1, const Vec4 &v2)
{
    float ret =
        (v1.X() * v2.X()) +
        (v1.Y() * v2.Y()) +
        (v1.Z() * v2.Z());

    SANITY_CHECK_FLOAT(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 Cross(const Vec4& v1, const Vec4& v2)
{
    Vec4 ret(
         (v1.Y()*v2.Z() - v1.Z()*v2.Y()),
        -(v1.X()*v2.Z() - v1.Z()*v2.X()),
         (v1.X()*v2.Y() - v1.Y()*v2.X())
    );

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator+=(const Vec4& v)
{
    X() += v.X();
    Y() += v.Y();
    Z() += v.Z();

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator-=(const Vec4& v)
{
    X() -= v.X();
    Y() -= v.Y();
    Z() -= v.Z();

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator*=(const Vec4& v)
{
    X() *= v.X();
    Y() *= v.Y();
    Z() *= v.Z();

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator/=(const Vec4& v)
{
    X() /= v.X();
    Y() /= v.Y();
    Z() /= v.Z();

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator*=(const float t)
{
    X() *= t;
    Y() *= t;
    Z() *= t;

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4& Vec4::operator/=(const float t)
{
    float k = 1.0f / t;
    X() *= k;
    Y() *= k;
    Z() *= k;

    VEC3_SANITY_CHECK((*this));

    return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 UnitVector(const Vec4& v)
{
    Vec4 ret = v / v.Length();

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 Reflect(const Vec4& v, const Vec4& n)
{
    Vec4 ret = v - (2 * Dot(v, n) * n);

    VEC3_SANITY_CHECK(ret);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline bool Refract(const Vec4& v, const Vec4& n, float ni_over_nt, Vec4& refracted)
{
    Vec4 uv = UnitVector(v);
    float dt = Dot(uv, n);
    float discriminant = 1.0f - ni_over_nt * ni_over_nt*(1 - dt * dt);
    if (discriminant > 0)
    {
        refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
        VEC3_SANITY_CHECK(refracted);
        return true;
    }
    else
    {
        return false;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec4 RotateVectorByQuaternion(const Vec4& v, const Vec4& q)
{
    Vec4 u(q[0], q[1], q[2]);
    float s = q[3];

    Vec4 vRet = 2.0f * Dot(u, v) * u
        + (s*s - Dot(u, u)) * v
        + 2.0f * s * Cross(u, v);

    VEC3_SANITY_CHECK(vRet);

    return vRet;
}
