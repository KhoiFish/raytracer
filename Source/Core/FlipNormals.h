#pragma once

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"

// ----------------------------------------------------------------------------------------------------------------------------

class FlipNormals : public IHitable
{
public:

	FlipNormals(IHitable* hitable) : Hitable(hitable) {}

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
	{
		if (Hitable->Hit(r, tMin, tMax, rec))
		{
			rec.Normal = -rec.Normal;
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual bool BoundingBox(float t0, float t1, AABB& box) const
	{
		return Hitable->BoundingBox(t0, t1, box);
	}

private:

	IHitable* Hitable;
};