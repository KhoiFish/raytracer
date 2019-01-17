#include "Sphere.h"

// ----------------------------------------------------------------------------------------------------------------------------

bool Sphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec3  oc = r.Origin() - Center;

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
            rec.Normal = (rec.P - Center) / Radius;
            rec.MatPtr = MatPtr;
            GetSphereUV((rec.P - Center) / Radius, rec.U, rec.V);
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
