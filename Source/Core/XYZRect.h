#pragma once

#include "IHitable.h"
#include "Material.h"
#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class XYZRect : public IHitable
{
public:

	enum AxisPlane
	{
		XY,
		XZ,
		YZ
	};

	XYZRect(AxisPlane axis, float a0, float a1, float b0, float b1, float k, Material* mat)
		: AxisMode(axis)
		, A0(a0), A1(a1), B0(b0), B1(b1), K(k)
		, Mat(mat)
	{
	}

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
	{
		float t, a, b;
		Vec3 normal;
		switch (AxisMode)
		{
		case XY:
			t = (K - r.Origin().Z()) / r.Direction().Z();
			a = r.Origin().X() + t * r.Direction().X();
			b = r.Origin().Y() + t * r.Direction().Y();
			normal = Vec3(0, 0, 1);
			break;

		case XZ:
			t = (K - r.Origin().Y()) / r.Direction().Y();
			a = r.Origin().X() + t * r.Direction().X();
			b = r.Origin().Z() + t * r.Direction().Z();
			normal = Vec3(0, 1, 0);
			break;

		case YZ:
			t = (K - r.Origin().X()) / r.Direction().X();
			a = r.Origin().Y() + t * r.Direction().Y();
			b = r.Origin().Z() + t * r.Direction().Z();
			normal = Vec3(1, 0, 0);
			break;
		}

		if (t < tMin || t > tMax)
		{
			return false;
		}

		if (a < A0 || a > A1 || b < B0 || b > B1)
		{
			return false;
		}

		rec.U = (a - A0) / (A1 - A0);
		rec.V = (b - B0) / (B1 - B0);
		rec.T = t;
		rec.MatPtr = Mat;
		rec.P = r.PointAtParameter(t);
		rec.Normal = normal;

		return true;
	}

	virtual bool BoundingBox(float t0, float t1, AABB& box) const
	{
		switch (AxisMode)
		{
		case XY:
			box = AABB(Vec3(A0, B0, K - 0.0001f), Vec3(A1, B1, K + 0.0001f));
			break;

		case XZ:
			box = AABB(Vec3(A0, K - 0.0001f, B0), Vec3(A1, K + 0.0001f, B1));
			break;

		case YZ:
			box = AABB(Vec3(K - 0.0001f, A0, B0), Vec3(K + 0.0001f, A1, B1));
			break;
		}

		return true;
	}

private:

	AxisPlane AxisMode;
	float     A0, A1, B0, B1, K;
	Material* Mat;
};