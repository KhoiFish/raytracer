#pragma once

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableTranslate : public IHitable
{
public:

    inline HitableTranslate(IHitable* p, const Vec4& displacement) : HitObject(p), Offset(displacement) {}
    virtual ~HitableTranslate();

    virtual bool           Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool           BoundingBox(float t0, float t1, AABB& box) const;
    inline const IHitable* GetHitObject() const { return HitObject; }
    inline const Vec4&     GetOffset() const { return Offset;  }

private:

    IHitable* HitObject;
    Vec4 Offset;
};

// ----------------------------------------------------------------------------------------------------------------------------

class HitableRotateY : public IHitable
{
public:

    HitableRotateY(IHitable* obj, float angleDeg);
    virtual ~HitableRotateY();

    virtual bool           Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool           BoundingBox(float t0, float t1, AABB& box) const;
    inline float           GetAngleDegrees() const { return AngleDegrees; }
    inline const IHitable* GetHitObject() const { return HitObject; }

private:

    IHitable* HitObject;
    float     AngleDegrees;
    float     SinTheta;
    float     CosTheta;
    bool      HasBox;
    AABB      Bbox;
};
