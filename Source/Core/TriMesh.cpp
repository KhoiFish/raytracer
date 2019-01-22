#include "TriMesh.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 1)
struct STLTriangle
{
    float    Normal[3];
    float    Vert0[3];
    float    Vert1[3];
    float    Vert2[3];
    int16_t  AttrByteCount;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

TriMesh* TriMesh::CreateFromSTLFile(const char* filePath, Material* material)
{
    TriMesh* ret = nullptr;
    std::ifstream inputFile(filePath, std::ios::in, std::ios::binary);
    if (inputFile.is_open())
    {
        ret = new TriMesh();

        // Read the header, which we'll ignore for now
        uint8_t header[80];
        inputFile.read((char*)header, sizeof(header));
        
        uint32_t numTriangles;
        inputFile.read((char*)&numTriangles, sizeof(uint32_t));

        // Read in all triangle data
        std::vector<Triangle*> triList;
        for (int i = 0; i < (int)numTriangles; i++)
        {
            STLTriangle stlTri;
            inputFile.read((char*)&stlTri, sizeof(STLTriangle));

            float hackScale = 1;
            Triangle::Vertex v0 =
            {
                Vec3(stlTri.Vert0[0], stlTri.Vert0[1], -stlTri.Vert0[2]),
                Vec3(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v1 =
            {
                Vec3(stlTri.Vert1[0], stlTri.Vert1[1], -stlTri.Vert1[2]),
                Vec3(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v2 =
            {
                Vec3(stlTri.Vert2[0], stlTri.Vert2[1], -stlTri.Vert2[2]),
                Vec3(1, 1, 1),
                {0, 0}
            };

            triList.push_back(new Triangle(v0, v1, v2, material));
        }

        // Convert to regular array
        ret->TriArray     = new IHitable*[triList.size()];
        ret->NumTriangles = (int)triList.size();
        for (int i = 0; i < ret->NumTriangles; i++)
        {
            ret->TriArray[i] = triList[i];
        }

        // Build BVH tree
        ret->BVHHead = new BVHNode(ret->TriArray, ret->NumTriangles, 0, 0);
    }

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool TriMesh::BoundingBox(float t0, float t1, AABB& box) const
{
    return BVHHead->BoundingBox(t0, t1, box);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool TriMesh::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    return BVHHead->Hit(r, tMin, tMax, rec);
}
