#pragma once

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"
#include "Texture.h"
#include "Material.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class ConstantMedium : public IHitable
{
public:

	ConstantMedium(IHitable* boundary, float density, Texture* tex);

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
	virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

	IHitable* Boundary;
	float     Density;
	Material* PhaseFunction;
};