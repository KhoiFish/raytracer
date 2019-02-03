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

    HitableBox(const Vec4& p0, const Vec4& p1, Material* mat);
    virtual ~HitableBox();

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

    inline void  GetPoints(Vec4& minP, Vec4& maxP) const
    {
        minP = Pmin;
        maxP = Pmax;
    }

    inline const Material* GetMaterial() const { return Mat; }

private:

    Vec4      Pmin, Pmax;
    IHitable* HitList;
    Material* Mat;
};
