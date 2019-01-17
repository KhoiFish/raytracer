#include "FlipNormals.h"

// ----------------------------------------------------------------------------------------------------------------------------

bool FlipNormals::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    if (Hitable->Hit(r, tMin, tMax, rec))
    {
        rec.Normal = -rec.Normal;
        return true;
    }
    else
    {
        return false;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool FlipNormals::BoundingBox(float t0, float t1, AABB& box) const
{
    return Hitable->BoundingBox(t0, t1, box);
}
