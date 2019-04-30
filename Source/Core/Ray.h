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

#include "vcl/vector3d.h"

// ----------------------------------------------------------------------------------------------------------------------------
namespace Core
{
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

        inline Vec4  Origin() const                       { return A; }
        inline Vec4  Direction() const                    { return B; }
        inline float Time() const                         { return Timestamp; }
        inline Vec4  PointAtParameter(float t) const      { return A + (t * B); }

        inline Vec3f OriginFast() const                   { return FastA; }
        inline Vec3f DirectionFast() const                { return FastB; }
        inline Vec3f InverseDirectionFast() const         { return FastInvB; }
        inline Vec3f PointAtParameterFast(float t) const  { return FastA + (t * FastB); }

        inline const float* InverseDirectionArray() const { return InvBArray; }

    private:

        Vec4   A;
        Vec4   B;
        float  Timestamp;

        Vec3f  FastA, FastB, FastInvB;
        float  InvBArray[3];
    };
}