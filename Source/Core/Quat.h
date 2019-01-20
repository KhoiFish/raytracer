#pragma once

#include "Vec3.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Quat
{
public:

    Quat() : S(0), V(0, 0, 0) {}
    Quat(float s, const Vec3& v) : S(s), V(v) {}

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
        Vec3  imaginary = q.V * S + V * q.S + ::Cross(V, q.V);

        return Quat(scalar, imaginary);
    }

    inline Quat operator*(const Vec3& otherV) const
    {
        float sPart = -(V.X()*otherV.X() + V.Y()*otherV.Y() + V.Z()*otherV.Z());
        float xPart =  (S    *otherV.X() + V.Y()*otherV.Z() - V.Z()*otherV.Y());
        float yPart =  (S    *otherV.Y() + V.Z()*otherV.X() - V.X()*otherV.Z());
        float zPart =  (S    *otherV.Z() + V.X()*otherV.Y() - V.Y()*otherV.X());

        return Quat (sPart, Vec3(xPart, yPart, zPart));
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
        Vec3  imaginary = conjugateValue.V*(absoluteValue);

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

    static inline Vec3 RotateVector(const Vec3& vToRotate, const Vec3& axis, float angleDegrees)
    {
        Quat p(0, vToRotate);
        Vec3 normalizedAxis = Vec3(axis).MakeUnitVector();

        Quat q(angleDegrees, normalizedAxis);
        q.ToUnitNormQuaternion();

        Quat qInverse = q.Inverse();
        Quat rotated = q * p * q.Inverse();

        return rotated.V;
    }

    inline Vec3 RotateVector(const Vec3& vToRotate) const
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
    Vec3  V;
};