#include "Triangle.h"   

// ----------------------------------------------------------------------------------------------------------------------------

Triangle::Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Material* Mat)
{
    Vertices[0] = v0;
    Vertices[1] = v1;
    Vertices[2] = v2;
    MatPtr      = Mat;
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
            vMin[j] = GetMin<float>(vMin[j], Vertices[i].Vert[j]);
            vMax[j] = GetMax<float>(vMax[j], Vertices[i].Vert[j]);
        }
    }

    box = AABB(vMin, vMax);

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Triangle::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    const float EPSILON = 0.0000001f;
    const Vec4& vertex0 = Vertices[0].Vert;
    const Vec4& vertex1 = Vertices[1].Vert;
    const Vec4& vertex2 = Vertices[2].Vert;

    Vec4 edge1, edge2, h, s, q;
    float a, f, u, v;

    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    h = Cross(r.Direction(), edge2);
    a = Dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
    {
        // This ray is parallel to this triangle
        return false;
    }

    f = 1.0f / a;
    s = r.Origin() - vertex0;
    u = f * Dot(s, h);
    if (u < 0.0f || u > 1.0)
    {
        return false;
    }

    q = Cross(s, edge1);
    v = f * Dot(r.Direction(), q);
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    // At this stage we can compute t to find out where the intersection point is on the line
    float t = f * Dot(edge2, q);
    if (t > EPSILON && t > tMin && t < tMax)
    {
        Vec4 tempUV[3] =
        {
            Vec4(Vertices[0].UV[0], Vertices[0].UV[1], 0),
            Vec4(Vertices[1].UV[0], Vertices[1].UV[1], 0),
            Vec4(Vertices[2].UV[0], Vertices[2].UV[1], 0)
        };

        Vec4 texUv  = (1 - u - v) * tempUV[0] + u * tempUV[1] + v * tempUV[2];
        Vec4 normal = (1 - u - v) * Vertices[0].Normal + u * Vertices[1].Normal + v * Vertices[2].Normal;

        // Ray intersection
        rec.U       = texUv[0];
        rec.V       = texUv[1];
        rec.T       = t;
        rec.MatPtr  = MatPtr;
        rec.P       = r.PointAtParameter(t);
        rec.Normal  = normal;
        return true;
    }
    else
    {
        // This means that there is a line intersection but not a ray intersection.
        return false;
    }
}
