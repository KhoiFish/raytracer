#pragma once

#include "IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class HitableList : public IHitable
{
public:

	inline HitableList(IHitable **l, int n) : List(l), ListSize(n) {}

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
	virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

	IHitable** List;
	int       ListSize;
};
