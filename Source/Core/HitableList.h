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
    class HitableList : public IHitable
    {
    public:

        inline HitableList(IHitable** l, int n, bool freeHitables = true) : List(l), ListSize(n), FreeHitables(freeHitables) {}
        virtual ~HitableList();

        virtual bool      Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
        virtual bool      BoundingBox(float t0, float t1, AABB& box) const;
        virtual float     PdfValue(const Vec4& origin, const Vec4& v) const;
        virtual Vec4      Random(const Vec4& origin) const;

        inline IHitable** GetList() const { return List; }
        inline const int  GetListSize() const { return ListSize; }

    private:

        bool       FreeHitables;
        IHitable** List;
        int        ListSize;
    };
}
