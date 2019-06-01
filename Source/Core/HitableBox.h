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
#include "FlipNormals.h"
#include "XYZRect.h"
#include "HitableList.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
    class HitableBox : public IHitable
    {
    public:

        HitableBox(const Vec4& p0, const Vec4& p1, Material* mat);
        virtual ~HitableBox();

        virtual bool BoundingBox(float t0, float t1, AABB& box) const;
        virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

        inline void  GetPoints(Vec4& minP, Vec4& maxP) const
        {
            minP = Pmin;
            maxP = Pmax;
        }

        virtual inline Material* GetMaterial() override { return Mat; }

    private:

        Vec4      Pmin, Pmax;
        IHitable* HitList;
        Material* Mat;
    };
}
