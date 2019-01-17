#include "HitableBox.h"

// ----------------------------------------------------------------------------------------------------------------------------

HitableBox::HitableBox(const Vec3& p0, const Vec3& p1, Material* Mat)
{
	Pmin = p0;
	Pmax = p1;
	IHitable** list = new IHitable*[6];

	int i = 0;
	list[i++] = new XYZRect(XYZRect::XY, p0.X(), p1.X(), p0.Y(), p1.Y(), p1.Z(), Mat);
	list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, p0.X(), p1.X(), p0.Y(), p1.Y(), p0.Z(), Mat));

	list[i++] = new XYZRect(XYZRect::XZ, p0.X(), p1.X(), p0.Z(), p1.Z(), p1.Y(), Mat);
	list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, p0.X(), p1.X(), p0.Z(), p1.Z(), p0.Y(), Mat));

	list[i++] = new XYZRect(XYZRect::YZ, p0.Y(), p1.Y(), p0.Z(), p1.Z(), p1.X(), Mat);
	list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, p0.Y(), p1.Y(), p0.Z(), p1.Z(), p0.X(), Mat));

	HitList = new HitableList(list, 6);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableBox::BoundingBox(float t0, float t1, AABB& box) const
{
	box = AABB(Pmin, Pmax);
	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool HitableBox::Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const
{
	return HitList->Hit(r, tMin, tMax, rec);
}
