// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "TriMesh.h"
#include <cstring>
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

TriMesh::~TriMesh()
{
    if (BVHHead != nullptr)
    {
        delete BVHHead;
        BVHHead = nullptr;
    }

    if (TriArray != nullptr)
    {
        delete[] TriArray;
        TriArray = nullptr;
    }

    if (Mat != nullptr)
    {
        delete Mat;
        Mat = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

TriMesh* TriMesh::CreateFromSTLFile(const char* filePath, Material* material, float scale /*= 1.0f*/)
{
    TriMesh* ret = nullptr;

    std::ifstream inputFile(filePath, std::ios::binary);
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
                Vec4(stlTri.Vert0[0], stlTri.Vert0[1], -stlTri.Vert0[2]) * scale,
                Vec4(0, 0, 0),
                Vec4(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v1 =
            {
                Vec4(stlTri.Vert1[0], stlTri.Vert1[1], -stlTri.Vert1[2]) * scale,
                Vec4(0, 0, 0),
                Vec4(1, 1, 1),
                {0, 0}
            };

            Triangle::Vertex v2 =
            {
                Vec4(stlTri.Vert2[0], stlTri.Vert2[1], -stlTri.Vert2[2]) * scale,
                Vec4(0, 0, 0),
                Vec4(1, 1, 1),
                {0, 0}
            };

            triList.push_back(new Triangle(v0, v1, v2, material));
        }

        ret->createFromArray(triList);
    }

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

Triangle* TriMesh::makeNewTriangle(
    int v0, int v1, int v2,
    const std::vector<Vec4>& vertList, const std::vector<Vec4>& vertNormalList, const std::vector<TexCoord>& texCoordList,
    const Face& face, Material* material)
{
    Triangle* newTri0 = new Triangle(
        Triangle::Vertex(
            {
                vertList[face.Verts[v0].VertIndex],
                face.Verts[v0].NormIndex >= 0 ? vertNormalList[face.Verts[v0].NormIndex] : Vec4(0, 0, 0),
                Vec4(0, 0, 0),
                {
                    face.Verts[v0].TexCoordIndex >= 0 ? texCoordList[face.Verts[v0].TexCoordIndex].UV[0] : 0.f,
                    face.Verts[v0].TexCoordIndex >= 0 ? texCoordList[face.Verts[v0].TexCoordIndex].UV[1] : 0.f,
                }
            }
        ),
        Triangle::Vertex(
            {
                vertList[face.Verts[v1].VertIndex],
                face.Verts[v1].NormIndex >= 0 ? vertNormalList[face.Verts[v1].NormIndex] : Vec4(0, 0, 0),
                Vec4(0, 0, 0),
                {
                    face.Verts[v1].TexCoordIndex >= 0 ? texCoordList[face.Verts[v1].TexCoordIndex].UV[0] : 0.f,
                    face.Verts[v1].TexCoordIndex >= 0 ? texCoordList[face.Verts[v1].TexCoordIndex].UV[1] : 0.f,
                }
            }
        ),
        Triangle::Vertex(
            {
                vertList[face.Verts[v2].VertIndex],
                face.Verts[v2].NormIndex >= 0 ? vertNormalList[face.Verts[v2].NormIndex] : Vec4(0, 0, 0),
                Vec4(0, 0, 0),
                {
                    face.Verts[v2].TexCoordIndex >= 0 ? texCoordList[face.Verts[v2].TexCoordIndex].UV[0] : 0.f,
                    face.Verts[v2].TexCoordIndex >= 0 ? texCoordList[face.Verts[v2].TexCoordIndex].UV[1] : 0.f,
                }
            }
        ),
        material
    );

    return newTri0;
}

// ----------------------------------------------------------------------------------------------------------------------------

TriMesh* TriMesh::CreateFromOBJFile(const char* filePath, float scale, bool makeMetalMaterial, Material* matOverride)
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

        std::vector<Vec4>       vertList;
        std::vector<Vec4>       vertNormalList;
        std::vector<TexCoord>   texCoordList;
        std::vector<Face>       faceList;

        const char* matLibKeyword = "mtllib";
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
                    const int lineBufOffset = (readMode == Vertex) ? 2 : 3;
                    std::string sourceString = &lineBuf[lineBufOffset];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

                    Vec4 point(
                         (float)atof(tokens[0].c_str()),
                         (float)atof(tokens[1].c_str()),
                         (float)atof(tokens[2].c_str()));

                    if (readMode == Vertex)
                    {
                        vertList.push_back(point * scale);
                    }
                    else
                    {
                        vertNormalList.push_back(UnitVector(point));
                    }
                }
                break;

                case TextureCoord:
                {
                    std::string sourceString = &lineBuf[3];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

                    RTL_ASSERT(tokens.size() >= 2);

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
                        if (faceCompTokens.size() < 3)
                        {
                            continue;
                        }

                        FaceVertex fc;
                        fc.VertIndex     = (faceCompTokens.size() > 0) ? atoi(faceCompTokens[0].c_str()) - 1 : 0;
                        fc.TexCoordIndex = (faceCompTokens.size() > 1) ? atoi(faceCompTokens[1].c_str()) - 1 : 0;
                        fc.NormIndex     = (faceCompTokens.size() > 2) ? atoi(faceCompTokens[2].c_str()) - 1 : 0;

                        face.Verts.push_back(fc);
                    }

                    if (face.Verts.size() >= 3)
                    {
                        faceList.push_back(face);
                    }
                }
                break;

                case MaterialLib:
                {
                    const int keyStrLen = (int)strlen(matLibKeyword);
                    std::string sourceString = &lineBuf[keyStrLen];
                    std::vector<std::string> tokens = GetStringTokens(sourceString, " ");

                    RTL_ASSERT(tokens.size() == 2);
                    if (matOverride == nullptr)
                    {
                        std::string matPath = GetAbsolutePath(GetParentDir(filePath) + std::string("/") + tokens[1]);
                        ret->Mat = new MWavefrontObj(matPath.c_str(), makeMetalMaterial);
                    }
                }
                break;

                default:
                break;
            }
        }

        // Material override?
        if (matOverride != nullptr)
        {
            ret->Mat = matOverride;
        }

        std::vector<Triangle*> triList;
        for (int i = 0; i < (int)faceList.size(); i++)
        {
            const Face& face = faceList[i];
            triList.push_back(ret->makeNewTriangle(0, 1, 2, vertList, vertNormalList, texCoordList, face, ret->Mat));
            if (face.Verts.size() == 4)
            {
                triList.push_back(ret->makeNewTriangle(1, 2, 3, vertList, vertNormalList, texCoordList, face, ret->Mat));
            }
        }

        ret->createFromArray(triList);
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

void TriMesh::createFromArray(std::vector<Triangle*> triArray)
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
