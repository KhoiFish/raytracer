#pragma once

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableTranslate : public IHitable
{
public:

    inline HitableTranslate(IHitable* p, const Vec3& displacement) : HitObject(p), Offset(displacement) {}

    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

    IHitable* HitObject;
    Vec3 Offset;
};

// ----------------------------------------------------------------------------------------------------------------------------

class HitableRotateY : public IHitable
{
public:

    HitableRotateY(IHitable* obj, float angleDeg);

    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

    IHitable* HitObject;
    float     SinTheta;
    float     CosTheta;
    bool      HasBox;
    AABB      Bbox;
};
