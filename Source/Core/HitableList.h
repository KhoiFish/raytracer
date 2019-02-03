#pragma once

#include "IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableList : public IHitable
{
public:

    inline HitableList(IHitable **l, int n, bool freeHitables = true) : List(l), ListSize(n), FreeHitables(freeHitables) {}
    virtual ~HitableList();

    virtual bool      Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool      BoundingBox(float t0, float t1, AABB& box) const;
    virtual float     PdfValue(const Vec4& origin, const Vec4& v) const;
    virtual Vec4      Random(const Vec4& origin) const;

    inline IHitable** GetList() const { return List; }
    inline const int  GetListSize() const { return ListSize; }

private:

    bool       FreeHitables;
    IHitable** List;
    int        ListSize;
};
