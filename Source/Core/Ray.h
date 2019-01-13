#pragma once

#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Ray
{
public:

	Ray() {}

	Ray(const Vec3& a, const Vec3& b, float ti = 0.f)
		: A(a), B(b), Timestamp(ti)
	{}

	Vec3  Origin() const    { return A; }
	Vec3  Direction() const { return B; }
	float Time() const      { return Timestamp; }

	Vec3 PointAtParameter(float t) const
	{
		return A + (t * B);
	}

private:

	Vec3   A;
	Vec3   B;
	float  Timestamp;
};