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

#include "IHitable.h"
#include "Material.h"
#include "Vec4.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
    class XYZRect : public IHitable
    {
    public:

        enum AxisPlane
        {
            XY,
            XZ,
            YZ
        };

        XYZRect(AxisPlane axis, float a0, float a1, float b0, float b1, float k, Material* mat, bool isLightShape = false);
        virtual ~XYZRect();

        virtual bool        Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
        virtual bool        BoundingBox(float t0, float t1, AABB& box) const;
        virtual float       PdfValue(const Vec4& origin, const Vec4& v) const;
        virtual Vec4        Random(const Vec4& origin) const;

        void                GetPlaneData(Vec4 outPoints[4], Vec4& normal);
        void                GetParams(float& a0, float& a1, float& b0, float& b1, float& k) const;
        AxisPlane           GetAxisPlane() const { return AxisMode; }
        virtual Material*   GetMaterial() override { return Mat; }

    private:

        AxisPlane AxisMode;
        float     A0, A1, B0, B1, K;
        Material* Mat;
    };
}
