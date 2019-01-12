#pragma once

#include "hitable.h"

class sphere : public hitable
{
public:

	sphere() {}

	sphere(vec3 cen, float r, material* mat) : center(cen), radius(r), mat_ptr(mat) {}
	virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;

private:

	vec3 center;
	float radius;
	material* mat_ptr;
};

bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const
{
	vec3 oc = r.origin() - center;

	float a = dot(r.direction(), r.direction());
	float b = dot(oc, r.direction());
	float c = dot(oc, oc) - radius * radius;
	float discriminant = (b * b) - (a * c);
	if (discriminant > 0)
	{
		float calc0 = sqrt((b*b) - (a*c));
		float test0 = (-b - calc0) / a;
		float test1 = (-b + calc0) / a;
		bool test0Passed = (test0 < t_max && test0 > t_min);
		bool test1Passed = (test1 < t_max && test1 > t_min);
		if (test0Passed || test1Passed)
		{
			rec.t = test0Passed ? test0 : test1;
			rec.p = r.point_at_parameter(rec.t);
			rec.normal = (rec.p - center) / radius;
			rec.mat_ptr = mat_ptr;
			return true;
		}
	}

	return false;
}