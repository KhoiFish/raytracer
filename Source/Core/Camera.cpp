#include "Camera.h"

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera()
    : Origin(0, 0, 0)
    , LowerLeftCorner(0, 0, 0)
    , Horizontal(0, 0, 0)
    , Vertical(0, 0, 0)
    , U(0, 0, 0)
    , V(0, 0, 0)
    , W(0, 0, 0)
    , Time0(0), Time1(0)
    , LensRadius(0)
    , ClearColor(0, 0, 0)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1, Vec3 clearColor)
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

    ClearColor = clearColor;
}
