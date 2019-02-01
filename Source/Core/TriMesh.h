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

    struct FaceVertex
    {
        int VertIndex;
        int TexCoordIndex;
        int NormIndex;
    };

    struct TexCoord
    {
        float UV[2];
    };

    struct Face
    {
        std::vector<FaceVertex> Verts;
    };

public:

    static TriMesh*               CreateFromSTLFile(const char* filePath, Material* material, float scale = 1.0f);
    static TriMesh*               CreateFromOBJFile(const char* filePath, float scale = 1.0f, bool makeMetalMaterial = false, Material* matOverride = nullptr);
    virtual bool                  BoundingBox(float t0, float t1, AABB& box) const;
    virtual bool                  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

    void GetTriArray(IHitable**& ppTriArray, int& numTris) const
    {
        ppTriArray = TriArray;
        numTris = NumTriangles;
    }

    const Material* GetMaterial() const { return Mat; }

private:

    TriMesh() : TriArray(nullptr), NumTriangles(0), BVHHead(NULL) {}

    void        createFromArray(std::vector<Triangle*> triArray);
    Triangle*   makeNewTriangle(
        int v0, int v1, int v2, 
        const std::vector<Vec3>& vertList, const std::vector<Vec3>& vertNormalList, const std::vector<TexCoord>& texCoordList,
        const Face& face, Material* material);

private:

    Material*          Mat;
    IHitable**         TriArray;
    int                NumTriangles;
    BVHNode*           BVHHead;
};
