#pragma once

#include "IHitable.h"
#include "Vec3.h"
#include "Material.h"
#include "Triangle.h"
#include "BVHNode.h"
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

class TriMesh : public IHitable
{
public:

    static TriMesh* CreateFromSTLFile(const char* filePath, Material* material, float scale = 1.0f);
    static TriMesh* CreateFromOBJFile(const char* filePath, Material* material, float scale = 1.0f);
    virtual bool    BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool    Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

private:

    TriMesh() : TriArray(nullptr), NumTriangles(0), BVHHead(NULL) {}

    void CreateFromArray(std::vector<Triangle*> triArray);


private:

    IHitable** TriArray;
    int        NumTriangles;
    BVHNode*   BVHHead;
};
