#pragma once
#include "IHitable.h"
#include "Vec3.h"
#include "Material.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Triangle : public IHitable
{
public:

    struct Vertex
    {
        Vec3  Vert;
        Vec3  Normal;
        Vec3  Color;
        float UV[2];
    };

public:

    Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Material* Mat);

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

private:

    Vertex      Vertices[3];
    Material*   MatPtr;
};
