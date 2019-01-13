#pragma once
#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

inline float RandomFloat()
{
	float num = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	return num;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomInUnitSphere()
{
	Vec3 p;
	do
	{
		p = 2.0f * Vec3(RandomFloat(), RandomFloat(), RandomFloat()) - Vec3(1.f, 1.f, 1.f);
	} while (p.SquaredLength() >= 1.f);

	return p;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 RandomInUnitDisk()
{
	Vec3 p;
	do 
	{
		p = 2.0f * Vec3(RandomFloat(), RandomFloat(), 0) - Vec3(1, 1, 0);
	} while (Dot(p, p) >= 1.0f);

	return p;
}

// ----------------------------------------------------------------------------------------------------------------------------

float Schlick(float cosine, float refIdx)
{
	float r0 = (1 - refIdx) / (1 + refIdx);
	r0 = r0 * r0;
	return r0 + (1 - r0)*pow(1 - cosine, 5);
}