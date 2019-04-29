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

#include "ConstantMedium.h"

// ----------------------------------------------------------------------------------------------------------------------------

ConstantMedium::ConstantMedium(IHitable* boundary, float density, BaseTexture* tex) : Boundary(boundary), Density(density)
{
    PhaseFunction = new MIsotropic(tex);
}

// ----------------------------------------------------------------------------------------------------------------------------

ConstantMedium::~ConstantMedium()
{
    if (PhaseFunction != nullptr)
    {
        delete PhaseFunction;
        PhaseFunction = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool ConstantMedium::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    HitRecord rec1, rec2;

    if (Boundary->Hit(r, -FLT_MAX, FLT_MAX, rec1))
    {
        if (Boundary->Hit(r, rec1.T + 0.0001f, FLT_MAX, rec2))
        {
            if (rec1.T < tMin)
            {
                rec1.T = tMin;
            }

            if (rec2.T > tMax)
            {
                rec2.T = tMax;
            }

            if (rec1.T >= rec2.T)
            {
                return false;
            }

            if (rec1.T < 0)
            {
                rec1.T = 0;
            }

            float distanceInsideBoundary = (rec2.T - rec1.T) * r.Direction().Length();
            float hitDistance = -(1 / Density) * log(RandomFloat());
            if (hitDistance < distanceInsideBoundary)
            {
                rec.T = rec1.T + hitDistance / r.Direction().Length();
                rec.P = r.PointAtParameter(rec.T);
                rec.Normal = Vec4(1, 0, 0); // This is arbitrary
                rec.MatPtr = PhaseFunction;

                return true;
            }
        }
    }

    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool ConstantMedium::    BoundingBox(float t0, float t1, AABB& box) const
{
    return Boundary->BoundingBox(t0, t1, box);
}
