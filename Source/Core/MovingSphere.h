#pragma once

#include "IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class MovingSphere : public IHitable
{
public:

    MovingSphere(Vec4 center0, Vec4 center1, float time0, float time1, float r, Material* mat);
    virtual ~MovingSphere();

    virtual bool  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool  BoundingBox(float t0, float t1, AABB& box) const;
    Vec4          Center(float time) const;
    float         GetRadius() const { return Radius; }
    Material*     GetMaterial() const { return Mat; }

private:

    Vec4       Center0, Center1;
    float      Time0, Time1;
    float      Radius;
    Material*  Mat;
};