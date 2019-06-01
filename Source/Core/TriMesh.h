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

#pragma once

#include "IHitable.h"
#include "Vec4.h"
#include "Material.h"
#include "CoreTriangle.h"
#include "BVHNode.h"
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
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

        virtual Material* GetMaterial() override { return Mat; }

    private:

        TriMesh() : TriArray(nullptr), NumTriangles(0), BVHHead(NULL) {}
        virtual ~TriMesh();

        void        createFromArray(std::vector<Triangle*> triArray);
        Triangle*   makeNewTriangle(
            int v0, int v1, int v2, 
            const std::vector<Vec4>& vertList, const std::vector<Vec4>& vertNormalList, const std::vector<TexCoord>& texCoordList,
            const Face& face, Material* material);

    private:

        Material*          Mat;
        IHitable**         TriArray;
        int                NumTriangles;
        BVHNode*           BVHHead;
    };
}
