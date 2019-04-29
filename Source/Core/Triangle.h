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
