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

#include "Ray.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Material;

// ----------------------------------------------------------------------------------------------------------------------------

struct HitRecord
{
    float      T;
    Vec4       P;
    Vec4       Normal;
    Material*  MatPtr;
    float      U, V;
};

// ----------------------------------------------------------------------------------------------------------------------------

class IHitable
{
public:

    virtual ~IHitable() {}

    virtual bool  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const = 0;
    virtual bool  BoundingBox(float t0, float t1, AABB& box) const = 0;
    virtual float PdfValue(const Vec4& origin, const Vec4& v) const { return 0.f; }
    virtual Vec4  Random(const Vec4& origin) const { return Vec4(1, 0, 0); }
    virtual bool  IsALightShape() const { return IsLightShape; }

protected:

    IHitable() : IsLightShape(false) {}
    IHitable(bool isLightShape) : IsLightShape(isLightShape) {}

private:

    bool IsLightShape;
};
