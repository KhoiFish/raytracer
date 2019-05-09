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

#include "Vec4.h"
#include "Util.h"

namespace Core
{
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

        inline Quat operator -(const Quat & q) const
        {
            return Quat(S - q.S, V - q.V);
        }

        inline Quat operator *(const Quat & q) const
        {
            float scalar = S * q.S - ::Core::Dot(V, q.V);
            Vec4  imaginary = q.V * S + V * q.S + ::Core::Cross(V, q.V);

            return Quat(scalar, imaginary);
        }

        inline Quat operator*(const Vec4 & otherV) const
        {
            float sPart = -(V.X() * otherV.X() + V.Y() * otherV.Y() + V.Z() * otherV.Z());
            float xPart = (S * otherV.X() + V.Y() * otherV.Z() - V.Z() * otherV.Y());
            float yPart = (S * otherV.Y() + V.Z() * otherV.X() - V.X() * otherV.Z());
            float zPart = (S * otherV.Z() + V.X() * otherV.Y() - V.Y() * otherV.X());

            return Quat(sPart, Vec4(xPart, yPart, zPart));
        }

        inline float Dot(const Quat & q) const
        {
            return S * q.S + ::Core::Dot(V, q.V);
        }

        inline float Norm() const
        {
            return sqrt((S * S) + ::Core::Dot(V, V));
        }

        inline Quat Conjugate() const
        {
            return Quat(S, (V * -1.f));
        }

        inline Quat Inverse() const
        {
            float absoluteValue = Norm();
            absoluteValue *= absoluteValue;
            absoluteValue = 1.f / absoluteValue;

            Quat conjugateValue = Conjugate();

            float scalar = conjugateValue.S * (absoluteValue);
            Vec4  imaginary = conjugateValue.V * (absoluteValue);

            return Quat(scalar, imaginary);
        }

        inline Quat ToUnitNormQuaternion()
        {
            float angle = DegreesToRadians(S);

            V.MakeUnitVector();
            S = cosf(angle * 0.5f);
            V = V * sinf(angle * 0.5f);

            return (*this);
        }

        static inline Vec4 RotateVector(const Vec4 & vToRotate, const Vec4 & axis, float angleDegrees)
        {
            Quat p(0, vToRotate);
            Vec4 normalizedAxis = Vec4(axis).MakeUnitVector();

            Quat q(angleDegrees, normalizedAxis);
            q.ToUnitNormQuaternion();

            Quat qInverse = q.Inverse();
            Quat rotated = q * p * q.Inverse();

            return rotated.V;
        }

        inline Vec4 RotateVector(const Vec4 & vToRotate) const
        {
            return RotateVector(vToRotate, V, S);
        }

        // ----------------------------------------------------------------------------

        inline Quat operator *(float value) const
        {
            return Quat(S * value, V * value);
        }

        inline void operator +=(const Quat & q)
        {
            (*this) = (*this) + q;
        }

        inline void operator -=(const Quat & q)
        {
            (*this) = (*this) - q;
        }

        inline void operator *=(const Quat & q)
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
}