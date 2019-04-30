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

// ----------------------------------------------------------------------------------------------------------------------------
namespace Core
{
    class MovingSphere : public IHitable
    {
    public:

        MovingSphere(Vec4 center0, Vec4 center1, float time0, float time1, float r, Material* mat);
        virtual ~MovingSphere();

        virtual bool  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
        virtual bool  BoundingBox(float t0, float t1, AABB& box) const;
        Vec4          Center(float time) const;
        float         GetRadius() const { return Radius; }
        Material*     GetMaterial() const { return Mat; }

    private:

        Vec4       Center0, Center1;
        float      Time0, Time1;
        float      Radius;
        Material*  Mat;
    };
}