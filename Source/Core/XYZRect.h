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

    XYZRect(AxisPlane axis, float a0, float a1, float b0, float b1, float k, Material* mat, bool isLightShape = false);
    virtual ~XYZRect();

    virtual bool    Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
    virtual bool    BoundingBox(float t0, float t1, AABB& box) const;
    virtual float   PdfValue(const Vec3& origin, const Vec3& v) const;
    virtual Vec3    Random(const Vec3& origin) const;

    void            GetPlaneData(Vec3 outPoints[4], Vec3& normal);
    void            GetParams(float& a0, float& a1, float& b0, float& b1, float& k) const;
    AxisPlane       GetAxisPlane() const { return AxisMode; }
    const Material* GetMaterial() const { return Mat; }

private:

    AxisPlane AxisMode;
    float     A0, A1, B0, B1, K;
    Material* Mat;
};
