#include "TriMesh.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cassert>

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

TriMesh* TriMesh::CreateFromSTLFile(const char* filePath, Material* material, float scale /*= 1.0f*/)
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

            Triangle::Vertex v0 =
            {
                Vec3(stlTri.Vert0[0], stlTri.Vert0[1], -stlTri.Vert0[2]) * scale,
                Vec3(0, 0, 0),
                Vec3(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v1 =
            {
                Vec3(stlTri.Vert1[0], stlTri.Vert1[1], -stlTri.Vert1[2]) * scale,
                Vec3(0, 0, 0),
                Vec3(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v2 =
            {
                Vec3(stlTri.Vert2[0], stlTri.Vert2[1], -stlTri.Vert2[2]) * scale,
                Vec3(0, 0, 0),
                Vec3(1, 1, 1),
                {0, 0}
            };

            triList.push_back(new Triangle(v0, v1, v2, material));
        }

        ret->CreateFromArray(triList);
    }

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline std::vector<std::string> getTokens(std::string sourceStr, std::string delim)
{
    std::vector<std::string> tokenList;
    size_t last = 0, next = 0;
    while ((next = sourceStr.find(delim, last)) != std::string::npos)
    {
        tokenList.push_back(sourceStr.substr(last, next - last + 1));
        last = next + delim.length();
    }
    tokenList.push_back(sourceStr.substr(last, next - last + 1));
    
    return tokenList;
}

// ----------------------------------------------------------------------------------------------------------------------------

TriMesh* TriMesh::CreateFromOBJFile(const char* filePath, Material* material, float scale /*= 1.0f*/)
{
    const int LINE_SIZE = 4096;
    char lineBuf[LINE_SIZE];
    TriMesh* ret = nullptr;
    std::ifstream inputFile(filePath);

    enum ReadMode
    {
        None,
        Object,
        Vertex,
        TextureCoord,
        VertexNormal,
        Face
    };

    if (inputFile.is_open())
    {
        std::vector<Vec3> vertList;
        std::vector<Vec3> vertNormalList;
        std::vector<Triangle*> triList;
        int vertCount = 0;
        int texCoordCount = 0;
        while (!inputFile.eof())
        {
            inputFile.getline(lineBuf, LINE_SIZE);

            ReadMode readMode = None;
            if (lineBuf[0] == 'o')
            {
                readMode = Object;
            }
            else if (lineBuf[0] == 'v' && lineBuf[1] == 't')
            {
                readMode = TextureCoord;
            }
            else if (lineBuf[0] == 'v' && lineBuf[1] == 'n')
            {
                readMode = VertexNormal;
            }
            else if (lineBuf[0] == 'v')
            {
                readMode = Vertex;
            }
            else if (lineBuf[0] == 'f')
            {
                readMode = Face;
            }

            switch (readMode)
            {
                case Object:
                {
                    vertList.clear();
                }
                break;

                case Vertex:
                case VertexNormal:
                {
                    std::string sourceString = &lineBuf[2];
                    std::vector<std::string> tokens = getTokens(sourceString, " ");

                    Vec3 vert(
                         (float)atof(tokens[0].c_str()),
                         (float)atof(tokens[1].c_str()),
                         (float)atof(tokens[2].c_str()));

                    if (readMode == Vertex)
                    {
                        vertList.push_back(vert * scale);
                    }
                    else
                    {
                        vertNormalList.push_back(vert);
                    }
                }
                break;

                case Face:
                {
                    struct FaceVertex
                    {
                        int VertIndex;
                        int TexCoordIndex;
                        int NormIndex;
                    };

                    std::vector<FaceVertex> faceVertices;
                    std::string sourceString = &lineBuf[2];
                    std::vector<std::string> faceTokens = getTokens(sourceString, " ");
                    for (int i = 0; i < (int)faceTokens.size(); i++)
                    {
                        std::vector<std::string> faceCompTokens = getTokens(faceTokens[i], "/");

                        FaceVertex fc;
                        fc.VertIndex     = (faceCompTokens.size() > 0) ? atoi(faceCompTokens[0].c_str()) : 0;
                        fc.TexCoordIndex = (faceCompTokens.size() > 1) ? atoi(faceCompTokens[1].c_str()) : 0;
                        fc.NormIndex     = (faceCompTokens.size() > 2) ? atoi(faceCompTokens[2].c_str()) : 0;

                        faceVertices.push_back(fc);
                    }
                    assert(faceVertices.size() == 3);

                    // Make tri
                    Triangle* newTri0 = new Triangle(
                        Triangle::Vertex(
                            {
                                vertList[faceVertices[0].VertIndex],
                                vertNormalList[faceVertices[0].NormIndex],
                                Vec3(0, 0, 0),
                                { 0, 0 }
                            }
                        ),
                        Triangle::Vertex(
                            {
                                vertList[faceVertices[1].VertIndex],
                                vertNormalList[faceVertices[1].NormIndex],
                                Vec3(0, 0, 0),
                                { 0, 0 }
                            }
                        ),
                        Triangle::Vertex(
                            {
                                vertList[faceVertices[2].VertIndex],
                                vertNormalList[faceVertices[2].NormIndex],
                                Vec3(0, 0, 0),
                                { 0, 0 }
                            }
                        ),
                        material
                    );

                    triList.push_back(newTri0);
                }
                break;
            }
        }

        ret = new TriMesh();
        ret->CreateFromArray(triList);
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

// ----------------------------------------------------------------------------------------------------------------------------

void TriMesh::CreateFromArray(std::vector<Triangle*> triArray)
{
    // Convert to regular array
    TriArray = new IHitable*[triArray.size()];
    NumTriangles = (int)triArray.size();
    for (int i = 0; i < NumTriangles; i++)
    {
        TriArray[i] = triArray[i];
    }

    // Build BVH tree
    BVHHead = new BVHNode(TriArray, NumTriangles, 0, 0);
}
