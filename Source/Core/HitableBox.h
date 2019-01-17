#pragma once
#include "IHitable.h"
#include "FlipNormals.h"
#include "XYZRect.h"
#include "HitableList.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableBox : public IHitable
{
public:

    HitableBox(const Vec3& p0, const Vec3& p1, Material* Mat);

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

private:

    Vec3      Pmin, Pmax;
    IHitable* HitList;
};
