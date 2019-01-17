#include "Camera.h"

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1)
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
