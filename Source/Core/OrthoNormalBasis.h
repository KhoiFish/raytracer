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

// ----------------------------------------------------------------------------------------------------------------------------
namespace Core
{
    class OrthoNormalBasis
    {
    public:

        inline Vec4 operator[] (int i) const { return Axis[i]; }

        inline Vec4 U() const { return Axis[0]; }
        inline Vec4 V() const { return Axis[1]; }
        inline Vec4 W() const { return Axis[2]; }

        inline Vec4 Local(float a, float b, float c) const { return a * U() + b * V() + c * W(); }
        inline Vec4 Local(const Vec4& v) const { return v.X()* U() + v.Y() * V() + v.Z() * W(); }

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
}
