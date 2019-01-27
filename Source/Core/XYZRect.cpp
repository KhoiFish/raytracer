#include "XYZRect.h"

// ----------------------------------------------------------------------------------------------------------------------------

bool XYZRect::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec3  normal, planeP;
    float aParams[2], bParams[2];
    switch (AxisMode)
    {
        case XY:
        {
            normal     = Vec3(0, 0, 1);
            planeP     = Vec3(A0, B0, K);
            aParams[0] = r.Origin().X();
            aParams[1] = r.Direction().X();
            bParams[0] = r.Origin().Y();
            bParams[1] = r.Direction().Y();
        }
        break;

        case XZ:
        {
            normal     = Vec3(0, 1, 0);
            planeP     = Vec3(A0, K, B0);
            aParams[0] = r.Origin().X();
            aParams[1] = r.Direction().X();
            bParams[0] = r.Origin().Z();
            bParams[1] = r.Direction().Z();
        }
        break;

        case YZ:
        {
            normal     = Vec3(1, 0, 0);
            planeP     = Vec3(K, A0, B0);
            aParams[0] = r.Origin().Y();
            aParams[1] = r.Direction().Y();
            bParams[0] = r.Origin().Z();
            bParams[1] = r.Direction().Z();
        }
        break;
    }

    const float denom = Dot(normal, r.Direction());
    if (fabs(denom) < 0.00001f)
    {
        return false;
    }

    const float t = Dot(planeP - r.Origin(), normal) / denom;
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

    rec.U       = (a - A0) / (A1 - A0);
    rec.V       = (b - B0) / (B1 - B0);
    rec.T       = t;
    rec.P       = r.PointAtParameter(t);
    rec.MatPtr  = Mat;
    rec.Normal  = normal;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool XYZRect::BoundingBox(float t0, float t1, AABB& box) const
{
    switch (AxisMode)
    {
    case XY:
        box = AABB(Vec3(A0, B0, K - 0.0001f), Vec3(A1, B1, K + 0.0001f));
        break;

    case XZ:
        box = AABB(Vec3(A0, K - 0.0001f, B0), Vec3(A1, K + 0.0001f, B1));
        break;

    case YZ:
        box = AABB(Vec3(K - 0.0001f, A0, B0), Vec3(K + 0.0001f, A1, B1));
        break;
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float XYZRect::PdfValue(const Vec3& origin, const Vec3& v) const
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

Vec3 XYZRect::Random(const Vec3& origin) const
{
    const float a = A0 + RandomFloat() * (A1 - A0);
    const float b = B0 + RandomFloat() * (B1 - B0);
    const float c = K;

    Vec3 randomPoint;
    switch (AxisMode)
    {
        case XY:
        {
            randomPoint = Vec3(a, b, c);
        }
        break;

        case XZ:
        {
            randomPoint = Vec3(a, c, b);
        }
        break;

        case YZ:
        {
            randomPoint = Vec3(c, a, b);
        }
        break;
    }

    return randomPoint - origin;
}

// ----------------------------------------------------------------------------------------------------------------------------

void XYZRect::GetPlaneData(Vec3 outPoints[4], Vec3& normal)
{
    switch (AxisMode)
    {
        case XY:
        {
            normal       = Vec3(0, 0, 1);
            outPoints[0] = Vec3(A0, B0, K);
            outPoints[1] = Vec3(A1, B0, K);
            outPoints[2] = Vec3(A1, B1, K);
            outPoints[3] = Vec3(A0, B1, K);
        }
        break;

        case XZ:
        {
            normal       = Vec3(0, 1, 0);
            outPoints[0] = Vec3(A0, K, B0);
            outPoints[1] = Vec3(A1, K, B0);
            outPoints[2] = Vec3(A1, K, B1);
            outPoints[3] = Vec3(A0, K, B1);
        }
        break;

        case YZ:
        {
            normal       = Vec3(1, 0, 0);
            outPoints[0] = Vec3(K, A0, B0);
            outPoints[1] = Vec3(K, A1, B0);
            outPoints[2] = Vec3(K, A1, B1);
            outPoints[3] = Vec3(K, A0, B1);
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
