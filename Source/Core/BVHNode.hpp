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

#include "BVHNode.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

int BVHNode::boxCompare(const void* a, const void* b, ECompareMode mode)
{
    AABB boxLeft, boxRight;
    IHitable* ah = *(IHitable**)a;
    IHitable* bh = *(IHitable**)b;

    if (!ah->BoundingBox(0, 0, boxLeft) || !bh->BoundingBox(0, 0, boxRight))
    {
        std::cerr << "No bounding box in bvh_node constructor\n";
        return 0;
    }

    float val;
    switch (mode)
    {
    case CompareX:
        val = boxLeft.Min().X() - boxRight.Min().X();
        break;

    case CompareY:
        val = boxLeft.Min().Y() - boxRight.Min().Y();
        break;

    case CompareZ:
        val = boxLeft.Min().Z() - boxRight.Min().Z();
        break;

    default:
        val = 0;
    }

    if (val < 0.f)
    {
        return -1;
    }
    else if (val == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

int BVHNode::boxXCompare(const void* a, const void* b)
{
    return boxCompare(a, b, ECompareMode::CompareX);
}

// ----------------------------------------------------------------------------------------------------------------------------

int BVHNode::boxYCompare(const void* a, const void* b)
{
    return boxCompare(a, b, ECompareMode::CompareY);
}

// ----------------------------------------------------------------------------------------------------------------------------

int BVHNode::boxZCompare(const void* a, const void* b)
{
    return boxCompare(a, b, ECompareMode::CompareZ);
}

// ----------------------------------------------------------------------------------------------------------------------------

BVHNode::BVHNode(IHitable** list, int n, float time0, float time1)
{
    // Randomly choose an axis to split on
    int axis = int(3 * RandomFloat());
    if (axis == 0)
    {
        qsort(list, n, sizeof(IHitable*), BVHNode::boxXCompare);
    }
    else if (axis == 1)
    {
        qsort(list, n, sizeof(IHitable*), BVHNode::boxYCompare);
    }
    else
    {
        qsort(list, n, sizeof(IHitable*), BVHNode::boxZCompare);
    }

    // Construct children nodes
    if (n == 1)
    {
        // For one object, just dupe to left and right
        Left = Right = list[0];
    }
    else if (n == 2)
    {
        Left = list[0];
        Right = list[1];
    }
    else
    {
        int halfN = n / 2;
        Left = new BVHNode(list, halfN, time0, time1);
        Right = new BVHNode(list + halfN, n - halfN, time0, time1);
    }

    // Compute bounding box
    AABB boxLeft, boxRight;
    if (!Left->BoundingBox(time0, time1, boxLeft) || !Right->BoundingBox(time0, time1, boxRight))
    {
        std::cerr << "No bounding box in bvh_node constructor\n";
    }
    Box = AABB::SurroundingBox(boxLeft, boxRight);
}

// ----------------------------------------------------------------------------------------------------------------------------

BVHNode::~BVHNode()
{
    if (Left != nullptr)
    {
        delete Left;
    }

    if (Right != nullptr && Right != Left)
    {
        delete Right;
    }

    Left  = nullptr;
    Right = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool BVHNode::BoundingBox(float t0, float t1, AABB& box) const
{
    box = Box;
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool BVHNode::Hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const
{
    if (Box.Hit(ray, tMin, tMax))
    {
        HitRecord leftRec, rightRec;

        bool hitLeft = Left->Hit(ray, tMin, tMax, leftRec);
        bool hitRight = Right->Hit(ray, tMin, tMax, rightRec);

        if (hitLeft && hitRight)
        {
            if (leftRec.T < rightRec.T)
            {
                rec = leftRec;
            }
            else
            {
                rec = rightRec;
            }

            return true;
        }
        else if (hitLeft)
        {
            rec = leftRec;
            return true;
        }
        else if (hitRight)
        {
            rec = rightRec;
            return true;
        }
    }

    return false;
}
