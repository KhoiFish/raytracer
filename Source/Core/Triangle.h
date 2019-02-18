#pragma once
#include "IHitable.h"
#include "Vec4.h"
#include "Ray.h"
#include "Material.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Triangle : public IHitable
{
public:

    struct Vertex
    {
        Vec4  Vert;
        Vec4  Normal;
        Vec4  Color;
        float UV[2];
    };

public:

    Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Material* Mat);

    virtual bool BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

    const Vertex* GetVertices() const { return Vertices; }

private:

    struct FastVert
    {
        Vec3f  Vert;
        Vec3f  Normal;
        Vec3f  Color;
        Vec3f  UV;
    };

private:

    Vertex      Vertices[3];
    FastVert    FastVertices[3];
    Material*   MatPtr;
};
