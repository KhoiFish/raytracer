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

#include "XYZRect.h" 

// ----------------------------------------------------------------------------------------------------------------------------

XYZRect::XYZRect(AxisPlane axis, float a0, float a1, float b0, float b1, float k, Material* mat, bool isLightShape /*= false*/) : IHitable(isLightShape)
, AxisMode(axis)
, A0(a0), A1(a1), B0(b0), B1(b1), K(k)
, Mat(mat)
{
    if (Mat->Owner == nullptr)
    {
        Mat->Owner = this;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

XYZRect::~XYZRect()
{
    if (Mat != nullptr && Mat->Owner == this)
    {
        delete Mat;
        Mat = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool XYZRect::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec3f normal, planeP;
    float aParams[2], bParams[2];
    switch (AxisMode)
    {
        case XY:
        {
            normal     = Vec3f(0, 0, 1);
            planeP     = Vec3f(A0, B0, K);
            aParams[0] = r.Origin().X();
            aParams[1] = r.Direction().X();
            bParams[0] = r.Origin().Y();
            bParams[1] = r.Direction().Y();
        }
        break;

        case XZ:
        {
            normal     = Vec3f(0, 1, 0);
            planeP     = Vec3f(A0, K, B0);
            aParams[0] = r.Origin().X();
            aParams[1] = r.Direction().X();
            bParams[0] = r.Origin().Z();
            bParams[1] = r.Direction().Z();
        }
        break;

        case YZ:
        {
            normal     = Vec3f(1, 0, 0);
            planeP     = Vec3f(K, A0, B0);
            aParams[0] = r.Origin().Y();
            aParams[1] = r.Direction().Y();
            bParams[0] = r.Origin().Z();
            bParams[1] = r.Direction().Z();
        }
        break;
    }

    const float denom = dot_product(normal, r.DirectionFast());
    if (fabs(denom) < 0.00001f)
    {
        return false;
    }

    const float t = dot_product(planeP - r.OriginFast(), normal) / denom;
    const float a = aParams[0] + t * aParams[1];
    const float b = bParams[0] + t * bParams[1];

    SANITY_CHECK_FLOAT(t);
    SANITY_CHECK_FLOAT(a);
    SANITY_CHECK_FLOAT(b);

    if (t < tMin || t > tMax)
    {
        return false;
    }

    if (a < A0 || a > A1 || b < B0 || b > B1)
    {
        return false;
    }

    Vec4 slowNormal;
    normal.store(&slowNormal[0]);

    rec.U       = (a - A0) / (A1 - A0);
    rec.V       = (b - B0) / (B1 - B0);
    rec.T       = t;
    rec.P       = r.PointAtParameter(t);
    rec.MatPtr  = Mat;
    rec.Normal  = slowNormal;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool XYZRect::BoundingBox(float t0, float t1, AABB& box) const
{
    switch (AxisMode)
    {
    case XY:
        box = AABB(Vec4(A0, B0, K - 0.0001f), Vec4(A1, B1, K + 0.0001f));
        break;

    case XZ:
        box = AABB(Vec4(A0, K - 0.0001f, B0), Vec4(A1, K + 0.0001f, B1));
        break;

    case YZ:
        box = AABB(Vec4(K - 0.0001f, A0, B0), Vec4(K + 0.0001f, A1, B1));
        break;
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float XYZRect::PdfValue(const Vec4& origin, const Vec4& v) const
{
    HitRecord rec;
    if (this->Hit(Ray(origin, v), 0.001f, FLT_MAX, rec))
    {
        float area            = (A1 - A0) * (B1 - B0);
        float distanceSquared = rec.T * rec.T * v.SquaredLength();
        float cosine          = fabs(Dot(v, rec.Normal) / v.Length());
        float ret             = distanceSquared / (cosine * area);

        SANITY_CHECK_FLOAT(ret);
        return ret;
    }
    else
    {
        return 0.f;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 XYZRect::Random(const Vec4& origin) const
{
    const float a = A0 + RandomFloat() * (A1 - A0);
    const float b = B0 + RandomFloat() * (B1 - B0);
    const float c = K;

    Vec4 randomPoint;
    switch (AxisMode)
    {
        case XY:
        {
            randomPoint = Vec4(a, b, c);
        }
        break;

        case XZ:
        {
            randomPoint = Vec4(a, c, b);
        }
        break;

        case YZ:
        {
            randomPoint = Vec4(c, a, b);
        }
        break;
    }

    return randomPoint - origin;
}

// ----------------------------------------------------------------------------------------------------------------------------

void XYZRect::GetPlaneData(Vec4 outPoints[4], Vec4& normal)
{
    switch (AxisMode)
    {
        case XY:
        {
            normal       = Vec4(0, 0, 1);
            outPoints[0] = Vec4(A0, B0, K);
            outPoints[1] = Vec4(A1, B0, K);
            outPoints[2] = Vec4(A1, B1, K);
            outPoints[3] = Vec4(A0, B1, K);
        }
        break;

        case XZ:
        {
            normal       = Vec4(0, 1, 0);
            outPoints[0] = Vec4(A0, K, B0);
            outPoints[1] = Vec4(A1, K, B0);
            outPoints[2] = Vec4(A1, K, B1);
            outPoints[3] = Vec4(A0, K, B1);
        }
        break;

        case YZ:
        {
            normal       = Vec4(1, 0, 0);
            outPoints[0] = Vec4(K, A0, B0);
            outPoints[1] = Vec4(K, A1, B0);
            outPoints[2] = Vec4(K, A1, B1);
            outPoints[3] = Vec4(K, A0, B1);
        }
        break;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void XYZRect::GetParams(float& a0, float& a1, float& b0, float& b1, float& k) const
{
    a0 = A0;
    a1 = A1;
    b0 = B0;
    b1 = B1;
    k  = K;
}
