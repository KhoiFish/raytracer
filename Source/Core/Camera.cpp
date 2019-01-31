#include "Camera.h"

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera()
    : LookFrom(0, 0, 0)
    , LookAt(0, 0, 0)
    , Up(0, 0, 0)
    , VertFov(0)
    , Aspect(0)
    , Aperture(0)
    , FocusDist(0)
    , ClearColor(0, 0, 0)
    , Origin(0, 0, 0)
    , LowerLeftCorner(0, 0, 0)
    , Horizontal(0, 0, 0)
    , Vertical(0, 0, 0)
    , U(0, 0, 0)
    , V(0, 0, 0)
    , W(0, 0, 0)
    , Time0(0), Time1(0)
    , LensRadius(0)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1, Vec3 clearColor)
{
    Setup(lookFrom, lookAt, vup, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::Setup(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1, Vec3 clearColor)
{
    LookFrom         = lookFrom;
    LookAt           = lookAt;
    Up               = vup;
    VertFov          = vertFov;
    Aspect           = aspect;
    Aperture         = aperture;
    FocusDist        = focusDist;
    Time0            = t0;
    Time1            = t1;
    ClearColor       = clearColor;

    UpdateInternalSettings();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::SetAspect(float aspect)
{
    Aspect = aspect;
    UpdateInternalSettings();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::UpdateInternalSettings()
{
    const float theta      = VertFov * RT_PI / 180.f;
    const float halfHeight = tan(theta / 2);
    const float halfWidth  = Aspect * halfHeight;

    Origin           = LookFrom;
    W                = UnitVector(LookFrom - LookAt);
    U                = UnitVector(Cross(Up, W));
    V                = Cross(W, U);
    LensRadius       = Aperture / 2.f;
    LowerLeftCorner  = Origin - halfWidth * FocusDist * U - halfHeight * FocusDist * V - FocusDist * W;
    Horizontal       = 2 * halfWidth  * FocusDist * U;
    Vertical         = 2 * halfHeight * FocusDist * V;
}
