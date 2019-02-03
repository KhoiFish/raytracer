#pragma once

#include "Vec4.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Quat
{
public:

    Quat() : S(0), V(0, 0, 0) {}
    Quat(float s, const Vec4& v) : S(s), V(v) {}

    // ----------------------------------------------------------------------------

    inline Quat operator +(const Quat& q) const
    {
        return Quat(S + q.S, V + q.V);
    }

    inline Quat operator -(const Quat& q) const
    {
        return Quat(S - q.S, V - q.V);
    }

    inline Quat operator *(const Quat& q) const
    {
        float scalar = S * q.S - ::Dot(V, q.V);
        Vec4  imaginary = q.V * S + V * q.S + ::Cross(V, q.V);

        return Quat(scalar, imaginary);
    }

    inline Quat operator*(const Vec4& otherV) const
    {
        float sPart = -(V.X()*otherV.X() + V.Y()*otherV.Y() + V.Z()*otherV.Z());
        float xPart =  (S    *otherV.X() + V.Y()*otherV.Z() - V.Z()*otherV.Y());
        float yPart =  (S    *otherV.Y() + V.Z()*otherV.X() - V.X()*otherV.Z());
        float zPart =  (S    *otherV.Z() + V.X()*otherV.Y() - V.Y()*otherV.X());

        return Quat (sPart, Vec4(xPart, yPart, zPart));
    }

    inline float Dot(const Quat& q) const
    {
        return S * q.S + ::Dot(V, q.V);
    }

    inline float Norm() const
    {
        return sqrt((S * S) + ::Dot(V, V));
    }

    inline Quat Conjugate() const
    {
        return Quat(S, (V * -1.f));
    }

    inline Quat Inverse() const
    {
        float absoluteValue = Norm();
        absoluteValue *= absoluteValue;
        absoluteValue  = 1.f / absoluteValue;

        Quat conjugateValue = Conjugate();

        float scalar    = conjugateValue.S*(absoluteValue);
        Vec4  imaginary = conjugateValue.V*(absoluteValue);

        return Quat(scalar, imaginary);
    }

    inline Quat ToUnitNormQuaternion()
    {
        float angle = DegreesToRadians(S);

        V.MakeUnitVector();
        S = cosf(angle*0.5f);
        V = V * sinf(angle*0.5f);

        return (*this);
    }

    static inline Vec4 RotateVector(const Vec4& vToRotate, const Vec4& axis, float angleDegrees)
    {
        Quat p(0, vToRotate);
        Vec4 normalizedAxis = Vec4(axis).MakeUnitVector();

        Quat q(angleDegrees, normalizedAxis);
        q.ToUnitNormQuaternion();

        Quat qInverse = q.Inverse();
        Quat rotated = q * p * q.Inverse();

        return rotated.V;
    }

    inline Vec4 RotateVector(const Vec4& vToRotate) const
    {
        return RotateVector(vToRotate, V, S);
    }

    // ----------------------------------------------------------------------------

    inline Quat operator *(float value) const
    {
        return Quat(S * value, V * value);
    }

    inline void operator +=(const Quat& q)
    {
        (*this) = (*this) + q;
    }

    inline void operator -=(const Quat& q)
    {
        (*this) = (*this) - q;
    }

    inline void operator *=(const Quat& q)
    {
        (*this) = (*this) * q;
    }

    inline Quat operator *=(float value)
    {
        (*this) = (*this) * value;
    }

private:

    float S;
    Vec4  V;
};