#pragma once

#include "Ray.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Material;

// ----------------------------------------------------------------------------------------------------------------------------

struct HitRecord
{
    float      T;
    Vec3       P;
    Vec3       Normal;
    Material*  MatPtr;
    float      U, V;
};

// ----------------------------------------------------------------------------------------------------------------------------

class IHitable
{
public:

    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const = 0;
    virtual bool BoundingBox(float t0, float t1, AABB& box) const = 0;
};
