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

#include "Triangle.h"

// ----------------------------------------------------------------------------------------------------------------------------

Triangle::Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Material* Mat)
{
    Vertex temp[3] =
    {
        v0, v1, v2
    };

    for (int i = 0; i < 3; i++)
    {
        Vertices[i] = temp[i];

        FastVertices[i].Vert   = Vec3f(temp[i].Vert[0],   temp[i].Vert[1],   temp[i].Vert[2]);
        FastVertices[i].Normal = Vec3f(temp[i].Normal[0], temp[i].Normal[1], temp[i].Normal[2]);
        FastVertices[i].Color  = Vec3f(temp[i].Color[0],  temp[i].Color[1],  temp[i].Color[2]);
        FastVertices[i].UV     = Vec3f(temp[i].UV[0],     temp[i].UV[1],     1.f);
    }
    
    MatPtr = Mat;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Triangle::BoundingBox(float t0, float t1, AABB& box) const
{
    Vec4 vMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec4 vMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            vMin[j] = GetMin<float>(vMin[j], FastVertices[i].Vert[j]);
            vMax[j] = GetMax<float>(vMax[j], FastVertices[i].Vert[j]);
        }
    }

    box = AABB(vMin, vMax);

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Triangle::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    const float EPSILON = 0.0000001f;

    const Vec3f& vertex0 = FastVertices[0].Vert;
    const Vec3f& vertex1 = FastVertices[1].Vert;
    const Vec3f& vertex2 = FastVertices[2].Vert;

    Vec3f rayOrigin    = r.OriginFast();
    Vec3f rayDirection = r.DirectionFast();

    Vec3f edge1, edge2, h, s, q;

    float a, f, u, v;

    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    h = cross_product(rayDirection, edge2);
    a = dot_product(edge1, h);
    if (a > -EPSILON && a < EPSILON)
    {
        // This ray is parallel to this triangle
        return false;
    }

    f = 1.0f / a;
    s = rayOrigin - vertex0;
    u = f * dot_product(s, h);
    if (u < 0.0f || u > 1.0)
    {
        return false;
    }

    q = cross_product(s, edge1);
    v = f * dot_product(rayDirection, q);
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    // At this stage we can compute t to find out where the intersection point is on the line
    float t = f * dot_product(edge2, q);
    if (t > EPSILON && t > tMin && t < tMax)
    {
        const float w       = (1 - u - v);
        Vec3f       texUv   = w * FastVertices[0].UV     + u * FastVertices[1].UV     + v * FastVertices[2].UV;
        Vec3f       normal  = w * FastVertices[0].Normal + u * FastVertices[1].Normal + v * FastVertices[2].Normal;

        Vec4 slowNormal;
        normal.store(&slowNormal[0]);

        // Ray intersection
        rec.U       = texUv[0];
        rec.V       = texUv[1];
        rec.T       = t;
        rec.MatPtr  = MatPtr;
        rec.P       = r.PointAtParameter(t);
        rec.Normal  = slowNormal;
        return true;
    }
    else
    {
        // This means that there is a line intersection but not a ray intersection.
        return false;
    }
}
