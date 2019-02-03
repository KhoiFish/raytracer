#pragma once

#include "IHitable.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Sphere : public IHitable
{
public:

    Sphere(Vec3 cen, float r, Material* mat, bool isLightShape = false);
    virtual ~Sphere();

    virtual bool    Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool    BoundingBox(float t0, float t1, AABB& box) const;
    virtual float   PdfValue(const Vec3& origin, const Vec3& v) const;
    virtual Vec3    Random(const Vec3& origin) const;

    const Vec3&     GetCenter() const { return Center; }
    float           GetRadius() const { return Radius; }
    Material*       GetMaterial() const { return Mat; }

private:

    Vec3       Center;
    float      Radius;
    Material*  Mat;
};
