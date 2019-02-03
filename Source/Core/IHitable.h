#pragma once

#include "Ray.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Material;

// ----------------------------------------------------------------------------------------------------------------------------

struct HitRecord
{
    float      T;
    Vec4       P;
    Vec4       Normal;
    Material*  MatPtr;
    float      U, V;
};

// ----------------------------------------------------------------------------------------------------------------------------

class IHitable
{
public:

    virtual ~IHitable() {}

    virtual bool  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const = 0;
    virtual bool  BoundingBox(float t0, float t1, AABB& box) const = 0;
    virtual float PdfValue(const Vec4& origin, const Vec4& v) const { return 0.f; }
    virtual Vec4  Random(const Vec4& origin) const { return Vec4(1, 0, 0); }
    virtual bool  IsALightShape() const { return IsLightShape; }

protected:

    IHitable() : IsLightShape(false) {}
    IHitable(bool isLightShape) : IsLightShape(isLightShape) {}

private:

    bool IsLightShape;
};
