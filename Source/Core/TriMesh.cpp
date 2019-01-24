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

static inline Triangle* MakeNewTriangle(int v0, int v1, int v2, std::vector<Vec3> vertList, std::vector<Vec3> vertNormalList, std::vector<TexCoord> texCoordList, const Face& face, Material* material)
{
    Triangle* newTri0 = new Triangle(
        Triangle::Vertex(
            {
                vertList[face.Verts[v0].VertIndex],
                vertNormalList[face.Verts[v0].NormIndex],
                Vec3(0, 0, 0),
                {
                    texCoordList[face.Verts[v0].TexCoordIndex].UV[0],
                    texCoordList[face.Verts[v0].TexCoordIndex].UV[1]
                }
            }
        ),
        Triangle::Vertex(
            {
                vertList[face.Verts[v1].VertIndex],
                vertNormalList[face.Verts[v1].NormIndex],
                Vec3(0, 0, 0),
                {
                    texCoordList[face.Verts[v1].TexCoordIndex].UV[0],
                    texCoordList[face.Verts[v1].TexCoordIndex].UV[1]
                }
            }
        ),
        Triangle::Vertex(
            {
                vertList[face.Verts[v2].VertIndex],
                vertNormalList[face.Verts[v2].NormIndex],
                Vec3(0, 0, 0),
                {
                    texCoordList[face.Verts[v2].TexCoordIndex].UV[0],
                    texCoordList[face.Verts[v2].TexCoordIndex].UV[1]
                }
            }
        ),
        material
    );

    return newTri0;
}

// ----------------------------------------------------------------------------------------------------------------------------

TriMesh* TriMesh::CreateFromOBJFile(const char* filePath, float scale /*= 1.0f*/)
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
        FaceMode,
        MaterialLib,
    };

    if (inputFile.is_open())
    {
        ret = new TriMesh();

        const char* matLibKeyword = "mtllib";

        std::vector<Vec3>       vertList;
        std::vector<Vec3>       vertNormalList;
        std::vector<TexCoord>   texCoordList;
        std::vector<Face>       faceList;
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
                readMode = FaceMode;
            }
            else if (strstr(lineBuf, matLibKeyword) == lineBuf)
            {
                readMode = MaterialLib;
            }

            switch (readMode)
            {
                case Vertex:
                case VertexNormal:
                {
                    std::string sourceString = &lineBuf[2];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

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

                case TextureCoord:
                {
                    std::string sourceString = &lineBuf[3];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

                    RTL_ASSERT(tokens.size() == 2);

                    TexCoord texCoord =
                    {
                        {
                            (float)atof(tokens[0].c_str()),
                            (float)atof(tokens[1].c_str())
                        }
                    };
                    
                    texCoordList.push_back(texCoord);
                }
                break;

                case FaceMode:
                {
                    Face face;
                    std::string sourceString = &lineBuf[2];
                    std::vector<std::string> faceTokens = GetStringTokens(sourceString, " ");
                    for (int i = 0; i < (int)faceTokens.size(); i++)
                    {
                        std::vector<std::string> faceCompTokens = GetStringTokens(faceTokens[i], "/");

                        FaceVertex fc;
                        fc.VertIndex     = (faceCompTokens.size() > 0) ? atoi(faceCompTokens[0].c_str()) - 1 : 0;
                        fc.TexCoordIndex = (faceCompTokens.size() > 1) ? atoi(faceCompTokens[1].c_str()) - 1 : 0;
                        fc.NormIndex     = (faceCompTokens.size() > 2) ? atoi(faceCompTokens[2].c_str()) - 1 : 0;

                        //DEBUG_PRINTF("%d %d %d\n", fc.VertIndex, fc.TexCoordIndex, fc.NormIndex);

                        face.Verts.push_back(fc);
                    }
                    faceList.push_back(face);
                }
                break;

                case MaterialLib:
                {
                    const int keyStrLen = (int)strlen(matLibKeyword);
                    std::string sourceString = &lineBuf[keyStrLen];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

                    RTL_ASSERT(tokens.size() == 2);
                    ret->Mat = new MWavefrontObj(tokens[1].c_str());
                }
                break;
            }
        }


        std::vector<Triangle*> triList;
        for (int i = 0; i < (int)faceList.size(); i++)
        {
            const Face& face = faceList[i];
            triList.push_back(MakeNewTriangle(0, 1, 2, vertList, vertNormalList, texCoordList, face, ret->Mat));
            if (face.Verts.size() == 4)
            {
                triList.push_back(MakeNewTriangle(1, 2, 3, vertList, vertNormalList, texCoordList, face, ret->Mat));
            }
        }

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
