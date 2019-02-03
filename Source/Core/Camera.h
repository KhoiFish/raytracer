#pragma once

#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Camera
{
public:

    Camera();
    Camera(Vec4 lookFrom, Vec4 lookAt, Vec4 vup,
        float vertFov, float aspect, float aperture, float focusDist,
        float t0, float t1, Vec4 backgroundColor);

    void Setup(Vec4 lookFrom, Vec4 lookAt, Vec4 vup,
        float vertFov, float aspect, float aperture, float focusDist,
        float t0, float t1, Vec4 backgroundColor);

    void SetAspect(float aspect);
    void SetFocusDistanceToLookAt();

    inline Ray GetRay(float s, float t) const
    {
        Vec4  rd     = LensRadius * RandomInUnitDisk();
        Vec4  offset = U * rd.X() + V * rd.Y();
        float time = Time0 + RandomFloat()*(Time1 - Time0);

        return Ray(Origin + offset, LowerLeftCorner + (s * Horizontal) + (t * Vertical) - Origin - offset, time);
    }

    inline void GetShutterTime(float& time0, float& time1) const
    {
        time0 = Time0;
        time1 = Time1;
    }

    inline Vec4 GetBackgroundColor() const
    {
        return BackgroundColor;
    }

    inline float GetVertFov() const
    {
        return VertFov;
    }

    inline void GetCameraParams(
        Vec4& lookFrom, Vec4& lookAt, Vec4& up,
        float& vertFov, float& aspect, float& aperture, float& focusDist,
        float& t0, float& t1, Vec4& clearColor) const
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

    Vec4   LookFrom; 
    Vec4   LookAt; 
    Vec4   Up;
    float  VertFov;
    float  Aspect;
    float  Aperture;
    float  FocusDist;
    Vec4   BackgroundColor;

    Vec4   Origin;
    Vec4   LowerLeftCorner;
    Vec4   Horizontal;
    Vec4   Vertical;
    Vec4   U, V, W;
    float  Time0, Time1;
    float  LensRadius;
};