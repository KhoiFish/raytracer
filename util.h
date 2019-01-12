#pragma once
#include "vec3.h"

inline float drand48()
{
	float num = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	return num;
}

inline vec3 random_in_unit_sphere()
{
	vec3 p;
	do
	{
		p = 2.0f * vec3(drand48(), drand48(), drand48()) - vec3(1.f, 1.f, 1.f);
	} while (p.squared_length() >= 1.f);

	return p;
}

inline vec3 random_in_unit_disk()
{
	vec3 p;
	do 
	{
		p = 2.0f * vec3(drand48(), drand48(), 0) - vec3(1, 1, 0);
	} while (dot(p, p) >= 1.0f);

	return p;
}

float schlick(float cosine, float ref_idx)
{
	float r0 = (1 - ref_idx) / (1 + ref_idx);
	r0 = r0 * r0;
	return r0 + (1 - r0)*pow(1 - cosine, 5);
}