#include "ConstantMedium.h"

// ----------------------------------------------------------------------------------------------------------------------------

ConstantMedium::ConstantMedium(IHitable* boundary, float density, Texture* tex) : Boundary(boundary), Density(density)
{
	PhaseFunction = new MIsotropic(tex);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool ConstantMedium::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
	HitRecord rec1, rec2;

	if (Boundary->Hit(r, -FLT_MAX, FLT_MAX, rec1))
	{
		if (Boundary->Hit(r, rec1.T + 0.0001f, FLT_MAX, rec2))
		{
			if (rec1.T < tMin)
			{
				rec1.T = tMin;
			}

			if (rec2.T > tMax)
			{
				rec2.T = tMax;
			}

			if (rec1.T >= rec2.T)
			{
				return false;
			}

			if (rec1.T < 0)
			{
				rec1.T = 0;
			}

			float distanceInsideBoundary = (rec2.T - rec1.T) * r.Direction().Length();
			float hitDistance = -(1 / Density) * log(RandomFloat());
			if (hitDistance < distanceInsideBoundary)
			{
				rec.T = rec1.T + hitDistance / r.Direction().Length();
				rec.P = r.PointAtParameter(rec.T);
				rec.Normal = Vec3(1, 0, 0); // This is arbitrary
				rec.MatPtr = PhaseFunction;

				return true;
			}
		}
	}

	return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool ConstantMedium::	BoundingBox(float t0, float t1, AABB& box) const
{
	return Boundary->BoundingBox(t0, t1, box);
}
