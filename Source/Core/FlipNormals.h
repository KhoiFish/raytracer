#pragma once

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class FlipNormals : public IHitable
{
public:

	inline FlipNormals(IHitable* hitable) : Hitable(hitable) {}

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
	virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

	IHitable* Hitable;
};
