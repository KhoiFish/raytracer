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

#include "HitableList.h"
#include "Util.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

HitableList::~HitableList()
{
    if (FreeHitables)
    {
        for (int i = 0; i < ListSize; i++)
        {
            delete List[i];
            List[i] = nullptr;
        }
    }

    delete[] List;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableList::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
    HitRecord tempRec;
    bool      hitAnything = false;
    float     closestSoFar = tMax;
    for (int i = 0; i < ListSize; i++)
    {
        if (List[i]->Hit(r, tMin, closestSoFar, tempRec))
        {
            hitAnything = true;
            closestSoFar = tempRec.T;
            rec = tempRec;
        }
    }

    return hitAnything;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableList::BoundingBox(float t0, float t1, AABB& box) const
{
    if (ListSize < 1)
    {
        return false;
    }

    // Try to build a box for the first object
    AABB retBox;
    AABB tempBox;
    bool firstTrue = List[0]->BoundingBox(t0, t1, tempBox);
    if (!firstTrue)
    {
        return false;
    }
    else
    {
        retBox = tempBox;
    }

    // Try adding additional hitable bounding boxes
    for (int i = 1; i < ListSize; i++)
    {
        if (List[0]->BoundingBox(t0, t1, tempBox))
        {
            retBox = AABB::SurroundingBox(retBox, tempBox);
        }
        else
        {
            return false;
        }
    }

    box = retBox;
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float HitableList::PdfValue(const Vec4& origin, const Vec4& v) const
{
    float weight = 1.0f / ListSize;
    float sum    = 0;
    for (int i = 0; i < ListSize; i++)
        sum += weight * List[i]->PdfValue(origin, v);

    return sum;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 HitableList::Random(const Vec4& origin) const
{
    int index = rand() % ListSize;
    return List[index]->Random(origin);
}
