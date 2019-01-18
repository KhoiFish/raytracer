#pragma once

#include "IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableList : public IHitable
{
public:

    inline HitableList(IHitable **l, int n) : List(l), ListSize(n) {}

    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool BoundingBox(float t0, float t1, AABB& box) const;

    inline const IHitable** GetList() const { return (const IHitable**)List; }
    inline const int        GetListSize() const { return ListSize; }

private:

    IHitable** List;
    int        ListSize;
};
