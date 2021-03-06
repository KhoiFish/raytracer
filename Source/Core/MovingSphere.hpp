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

#include "MovingSphere.h"
#include "Util.h"
#include "Material.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

MovingSphere::MovingSphere(Vec4 center0, Vec4 center1, float time0, float time1, float r, Material* mat) : Center0(center0), Center1(center1)
, Time0(time0), Time1(time1)
, Radius(r), Mat(mat)
{
    if (Mat->Owner == nullptr)
    {
        Mat->Owner = this;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

MovingSphere::~MovingSphere()
{
    if (Mat != nullptr && Mat->Owner == this)
    {
        delete Mat;
        Mat = nullptr;
    }
}
// ----------------------------------------------------------------------------------------------------------------------------

Vec4 MovingSphere::Center(float time) const
{
    return Center0 + ((time - Time0) / (Time1 - Time0)) * (Center1 - Center0);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MovingSphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec4  oc = r.Origin() - Center(r.Time());

    float a = Dot(r.Direction(), r.Direction());
    float b = Dot(oc, r.Direction());
    float c = Dot(oc, oc) - Radius * Radius;
    float d = (b * b) - (a * c);
    if (d > 0)
    {
        float calc0 = sqrt((b*b) - (a*c));
        float test0 = (-b - calc0) / a;
        float test1 = (-b + calc0) / a;
        bool test0Passed = (test0 < tMax && test0 > tMin);
        bool test1Passed = (test1 < tMax && test1 > tMin);
        if (test0Passed || test1Passed)
        {
            rec.T = test0Passed ? test0 : test1;
            rec.P = r.PointAtParameter(rec.T);
            rec.Normal = (rec.P - Center(r.Time())) / Radius;
            rec.MatPtr = Mat;
            GetSphereUV((rec.P - Center(r.Time())) / Radius, rec.U, rec.V);
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MovingSphere::BoundingBox(float t0, float t1, AABB& box) const
{
    AABB a = AABB::ComputerAABBForSphere(Center0, Radius);
    AABB b = AABB::ComputerAABBForSphere(Center1, Radius);

    box = AABB::SurroundingBox(a, b);

    return true;
}
