#include "MovingSphere.h"
#include "Util.h"
#include "Material.h"

// ----------------------------------------------------------------------------------------------------------------------------

MovingSphere::MovingSphere(Vec3 center0, Vec3 center1, float time0, float time1, float r, Material* mat) : Center0(center0), Center1(center1)
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

Vec3 MovingSphere::Center(float time) const
{
    return Center0 + ((time - Time0) / (Time1 - Time0)) * (Center1 - Center0);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MovingSphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec3  oc = r.Origin() - Center(r.Time());

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
