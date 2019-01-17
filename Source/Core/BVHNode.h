#pragma once
#include "IHitable.h"
#include "AABB.h"
#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class BVHNode : public IHitable
{
public:

	BVHNode(IHitable** list, int n, float time0, float time1);

	virtual bool Hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
	virtual bool BoundingBox(float t0, float t1, AABB& box) const;

private:

	enum ECompareMode
	{
		CompareX, CompareY, CompareZ
	};

	static int boxCompare(const void* a, const void* b, ECompareMode mode);
	static int boxXCompare(const void* a, const void* b);
	static int boxYCompare(const void* a, const void* b);
	static int boxZCompare(const void* a, const void* b);

private:

	IHitable* Left;
	IHitable* Right;
	AABB      Box;
};
