#include "HitableList.h"

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
