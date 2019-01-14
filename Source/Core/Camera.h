#pragma once

#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Camera
{
public:

	Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, 
		float vertFov, float aspect, float aperture, float focusDist,
		float t0, float t1)
	{
		Time0 = t0;
		Time1 = t1;
		LensRadius = aperture / 2;	

		float theta      = vertFov * RT_PI / 180.f;
		float halfHeight = tan(theta / 2);
		float halfWidth  = aspect * halfHeight;

		Origin = lookFrom;
		W      = UnitVector(lookFrom - lookAt);
		U      = UnitVector(Cross(vup, W));
		V      = Cross(W, U);

		LowerLeftCorner = Origin - halfWidth * focusDist * U - halfHeight * focusDist * V - focusDist * W;
		Horizontal      = 2 * halfWidth * focusDist * U;
		Vertical        = 2 * halfHeight * focusDist * V;
	}

	Ray GetRay(float s, float t) const
	{
		Vec3  rd     = LensRadius * RandomInUnitDisk();
		Vec3  offset = U * rd.X() + V * rd.Y();
		float time = Time0 + RandomFloat()*(Time1 - Time0);

		return Ray(Origin + offset, LowerLeftCorner + (s * Horizontal) + (t * Vertical) - Origin - offset, time);
	}

private:

	Vec3   Origin;
	Vec3   LowerLeftCorner;
	Vec3   Horizontal;
	Vec3   Vertical;
	Vec3   U, V, W;
	float  Time0, Time1;
	float  LensRadius;
};