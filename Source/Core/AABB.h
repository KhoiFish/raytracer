#pragma once
#include "Ray.h"
#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class AABB
{
public:

	inline AABB() {}
	inline AABB(const Vec3& minP, const Vec3& maxP) : MinP(minP), MaxP(maxP) {}

	inline Vec3 Min() const { return MinP; }
	inline Vec3 Max() const { return MaxP; }

	inline bool Hit(const Ray& ray, float tMin, float tMax) const
	{
		for (int i = 0; i < 3; i++)
		{
			// Solve for t params
			float invD = 1.0f / ray.Direction()[i];
			float t0   = (Min()[i] - ray.Origin()[i]) * invD;
			float t1   = (Max()[i] - ray.Origin()[i]) * invD;
			if (invD < 0.f)
			{
				std::swap(t0, t1);
			}

			// Test for non-overlap and reject
			tMin = t0 > tMin ? t0 : tMin;
			tMax = t1 < tMax ? t1 : tMax;
			if (tMax < tMin)
			{
				return false;
			}
		}

		return true;
	}

	static inline AABB ComputerAABBForSphere(const Vec3& center, float radius)
	{
		Vec3 temp(radius, radius, radius);
		return AABB(center - temp, center + temp);
	}

	static inline AABB SurroundingBox(AABB box0, AABB box1)
	{
		Vec3 small(
			fmin(box0.MinP.X(), box1.MinP.X()),
			fmin(box0.MinP.Y(), box1.MinP.Y()),
			fmin(box0.MinP.Z(), box1.MinP.Z())
		);

		Vec3 big(
			fmax(box0.Max().X(), box1.Max().X()),
			fmax(box0.Max().Y(), box1.Max().Y()),
			fmax(box0.Max().Z(), box1.Max().Z())
		);

		return AABB(small, big);
	}

private:

	Vec3 MinP;
	Vec3 MaxP;
};
