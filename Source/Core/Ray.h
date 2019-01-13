#pragma once

#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Ray
{
public:

	Ray() {}

	Ray(const Vec3& a, const Vec3& b)
	{
		A = a;
		B = b;
	}

	Vec3  Origin() const    { return A; }
	Vec3  Direction() const { return B; }
	Vec3& Origin()          { return A; }
	Vec3& Direction()       { return B; }

	Vec3 PointAtParameter(float t) const
	{
		return A + (t * B);
	}

private:

	Vec3 A;
	Vec3 B;
};