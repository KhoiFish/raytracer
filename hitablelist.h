#pragma once

#include "Hitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableList : public Hitable
{
public:

	HitableList() {}
	HitableList(Hitable **l, int n) : List(l), ListSize(n) {}

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;

private:

	Hitable** List;
	int       ListSize;
};

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