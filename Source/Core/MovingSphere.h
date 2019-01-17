#pragma once

#include "IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

class MovingSphere : public IHitable
{
public:

	inline MovingSphere(Vec3 center0, Vec3 center1, float time0, float time1, float r, Material* mat)
		: Center0(center0), Center1(center1)
		, Time0(time0), Time1(time1)
		, Radius(r), MatPtr(mat) {}

	virtual bool  Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
	virtual bool  BoundingBox(float t0, float t1, AABB& box) const;
	Vec3          Center(float time) const;

private:

	Vec3       Center0, Center1;
	float      Time0, Time1;
	float      Radius;
	Material*  MatPtr;
};