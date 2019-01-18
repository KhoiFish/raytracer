#pragma once

#include "IHitable.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Sphere : public IHitable
{
public:

    inline Sphere(Vec3 cen, float r, Material* mat) : Center(cen), Radius(r), MatPtr(mat) {}

    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool BoundingBox(float t0, float t1, AABB& box) const;

    const Vec3& GetCenter() const { return Center; }
    float       GetRadius() const { return Radius; }

private:

    Vec3       Center;
    float      Radius;
    Material*  MatPtr;
};
