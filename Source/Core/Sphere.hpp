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

#include "Sphere.h"
#include "OrthoNormalBasis.h"
#include "Material.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

Sphere::Sphere(Vec4 cen, float r, Material* mat, bool isLightShape /*= false*/) : IHitable(isLightShape), Center(cen), Radius(r), Mat(mat)
{
    CenterFast = Vec3f(cen[0], cen[1], cen[2]);

    if (Mat->Owner == nullptr)
    {
        Mat->Owner = this;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Sphere::~Sphere()
{
    if (Mat != nullptr && Mat->Owner == this)
    {
        delete Mat;
        Mat = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Sphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec3f oc = r.OriginFast() - CenterFast;

    float a     = dot_product(r.DirectionFast(), r.DirectionFast());
    float b     = dot_product(oc, r.DirectionFast());
    float bSqrd = b * b;
    float c     = dot_product(oc, oc) - Radius * Radius;
    float d     = bSqrd - (a * c);
    if (d > 0)
    {
        const float oneOverA    = 1.f / a;
        const float calc0       = sqrt(bSqrd - (a*c));
        const float test0       = (-b - calc0) * oneOverA;
        const float test1       = (-b + calc0) * oneOverA;
        const bool  test0Passed = (test0 < tMax && test0 > tMin);
        const bool  test1Passed = (test1 < tMax && test1 > tMin);
        if (test0Passed || test1Passed)
        {
            rec.T = test0Passed ? test0 : test1;
            rec.P = r.PointAtParameter(rec.T);

            const Vec4 delta = (rec.P - Center) / Radius;
            rec.Normal = delta;
            rec.MatPtr = Mat;

            GetSphereUV(delta, rec.U, rec.V);

            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Sphere::BoundingBox(float t0, float t1, AABB& box) const
{
    box = AABB::ComputerAABBForSphere(Center, Radius);
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float Sphere::PdfValue(const Vec4& origin, const Vec4& v) const
{
    HitRecord rec;

    if (Hit(Ray(origin, v), 0.001f, FLT_MAX, rec))
    {
        float cosThetaMax = sqrt(1 - Radius * Radius / (Center - origin).SquaredLength());
        float solidAngle = 2 * RT_PI * (1 - cosThetaMax);

        return 1.f / solidAngle;
    }
    else
    {
        return 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 Sphere::Random(const Vec4& origin) const
{
    Vec4  direction = Center - origin;
    float distanceSquared = direction.SquaredLength();

    OrthoNormalBasis uvw;
    uvw.BuildFromW(direction);

    return uvw.Local(RandomToSphere(Radius, distanceSquared));
}
