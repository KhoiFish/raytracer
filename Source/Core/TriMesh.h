#pragma once

#include "IHitable.h"
#include "Vec3.h"
#include "Material.h"
#include "Triangle.h"
#include "BVHNode.h"

// ----------------------------------------------------------------------------------------------------------------------------

class TriMesh : public IHitable
{
public:

    static TriMesh* CreateFromSTLFile(const char* filePath, Material* material);
    virtual bool    BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool    Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

private:

    TriMesh() : TriArray(nullptr), NumTriangles(0), BVHHead(NULL) {}

private:

    IHitable** TriArray;
    int        NumTriangles;
    BVHNode*   BVHHead;
};
