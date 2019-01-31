#pragma once

#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Camera
{
public:

    Camera();
    Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup,
        float vertFov, float aspect, float aperture, float focusDist,
        float t0, float t1, Vec3 backgroundColor);

    void Setup(Vec3 lookFrom, Vec3 lookAt, Vec3 vup,
        float vertFov, float aspect, float aperture, float focusDist,
        float t0, float t1, Vec3 backgroundColor);

    void SetAspect(float aspect);
    void SetFocusDistanceToLookAt();

    inline Ray GetRay(float s, float t) const
    {
        Vec3  rd     = LensRadius * RandomInUnitDisk();
        Vec3  offset = U * rd.X() + V * rd.Y();
        float time = Time0 + RandomFloat()*(Time1 - Time0);

        return Ray(Origin + offset, LowerLeftCorner + (s * Horizontal) + (t * Vertical) - Origin - offset, time);
    }

    inline void GetShutterTime(float& time0, float& time1) const
    {
        time0 = Time0;
        time1 = Time1;
    }

    inline Vec3 GetBackgroundColor() const
    {
        return BackgroundColor;
    }

    inline void GetCameraParams(
        Vec3& lookFrom, Vec3& lookAt, Vec3& up,
        float& vertFov, float& aspect, float& aperture, float& focusDist,
        float& t0, float& t1, Vec3& clearColor) const
    {
        lookFrom = LookFrom;
        lookAt = LookAt;
        up = Up;
        vertFov = VertFov;
        aspect = Aspect;
        aperture = Aperture;
        focusDist = FocusDist;
        t0 = Time0;
        t1 = Time1;
        clearColor = BackgroundColor;
    }

private:

    void UpdateInternalSettings();

private:

    Vec3   LookFrom; 
    Vec3   LookAt; 
    Vec3   Up;
    float  VertFov;
    float  Aspect;
    float  Aperture;
    float  FocusDist;
    Vec3   BackgroundColor;

    Vec3   Origin;
    Vec3   LowerLeftCorner;
    Vec3   Horizontal;
    Vec3   Vertical;
    Vec3   U, V, W;
    float  Time0, Time1;
    float  LensRadius;
};