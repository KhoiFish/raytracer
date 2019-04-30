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

#include "HitableTransform.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

HitableTranslate::~HitableTranslate()
{
    if (HitObject != nullptr)
    {
        delete HitObject;
        HitObject = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableTranslate::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Ray movedRay(r.Origin() - Offset, r.Direction(), r.Time());
    if (HitObject->Hit(movedRay, tMin, tMax, rec))
    {
        rec.P += Offset;
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableTranslate::BoundingBox(float t0, float t1, AABB& box) const
{
    if (HitObject->BoundingBox(t0, t1, box))
    {
        box = AABB(box.Min() + Offset, box.Max() + Offset);
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

HitableRotateY::HitableRotateY(IHitable* obj, float angleDeg) : HitObject(obj), AngleDegrees(angleDeg)
{
    float radians = (RT_PI / 180.f) * angleDeg;

    SinTheta = sin(radians);
    CosTheta = cos(radians);
    HasBox = HitObject->BoundingBox(0, 1, Bbox);

    Vec4 minV(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec4 maxV(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                float x = i * Bbox.Max().X() + (1 - i) * Bbox.Min().X();
                float y = j * Bbox.Max().Y() + (1 - j) * Bbox.Min().Y();
                float z = k * Bbox.Max().Z() + (1 - k) * Bbox.Min().Z();
                float newX =  CosTheta * x + SinTheta * z;
                float newZ = -SinTheta * x + CosTheta * z;

                Vec4 tester(newX, y, newZ);
                for (int c = 0; c < 3; c++)
                {
                    if (tester[c] > maxV[c])
                    {
                        maxV[c] = tester[c];
                    }
                    if (tester[c] < minV[c])
                    {
                        minV[c] = tester[c];
                    }
                }
            }
        }
    }
    Bbox = AABB(minV, maxV);
}

// ----------------------------------------------------------------------------------------------------------------------------

HitableRotateY::~HitableRotateY()
{
    if (HitObject != nullptr)
    {
        delete HitObject;
        HitObject = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableRotateY::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    Vec4 origin = r.Origin();
    Vec4 direction = r.Direction();

    origin[0] = CosTheta * r.Origin()[0] - SinTheta * r.Origin()[2];
    origin[2] = SinTheta * r.Origin()[0] + CosTheta * r.Origin()[2];

    direction[0] = CosTheta * r.Direction()[0] - SinTheta * r.Direction()[2];
    direction[2] = SinTheta * r.Direction()[0] + CosTheta * r.Direction()[2];

    Ray rotatedR(origin, direction, r.Time());
    if (HitObject->Hit(rotatedR, tMin, tMax, rec))
    {
        Vec4 p = rec.P;
        Vec4 normal = rec.Normal;

        p[0] =  CosTheta * rec.P[0] + SinTheta * rec.P[2];
        p[2] = -SinTheta * rec.P[0] + CosTheta * rec.P[2];

        normal[0] =  CosTheta * rec.Normal[0] + SinTheta * rec.Normal[2];
        normal[2] = -SinTheta * rec.Normal[0] + CosTheta * rec.Normal[2];

        rec.P = p;
        rec.Normal = normal;

        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableRotateY::BoundingBox(float t0, float t1, AABB& box) const
{
    box = Bbox;
    return HasBox;
}
