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

#include "HitableBox.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

HitableBox::HitableBox(const Vec4& p0, const Vec4& p1, Material* mat) : Mat(mat)
{
    if (Mat->Owner == nullptr)
    {
        Mat->Owner = this;
    }

    Pmin = p0;
    Pmax = p1;
    IHitable** list = new IHitable*[6];

    int i = 0;
    list[i++] = new XYZRect(XYZRect::XY, p0.X(), p1.X(), p0.Y(), p1.Y(), p1.Z(), mat);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, p0.X(), p1.X(), p0.Y(), p1.Y(), p0.Z(), mat));

    list[i++] = new XYZRect(XYZRect::XZ, p0.X(), p1.X(), p0.Z(), p1.Z(), p1.Y(), mat);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, p0.X(), p1.X(), p0.Z(), p1.Z(), p0.Y(), mat));

    list[i++] = new XYZRect(XYZRect::YZ, p0.Y(), p1.Y(), p0.Z(), p1.Z(), p1.X(), mat);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, p0.Y(), p1.Y(), p0.Z(), p1.Z(), p0.X(), mat));

    HitList = new HitableList(list, 6);
}

// ----------------------------------------------------------------------------------------------------------------------------

HitableBox::~HitableBox()
{
   if (HitList != nullptr)
   {
       delete HitList;
       HitList = nullptr;
   }

   if (Mat != nullptr && Mat->Owner == this)
   {
       delete Mat;
       Mat = nullptr;
   }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableBox::BoundingBox(float t0, float t1, AABB& box) const
{
    box = AABB(Pmin, Pmax);
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableBox::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    return HitList->Hit(r, tMin, tMax, rec);
}
