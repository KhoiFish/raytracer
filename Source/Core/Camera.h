#pragma once

#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Camera
{
public:

    Camera(Vec3 lookFrom, Vec3 lookAt, Vec3 vup, 
        float vertFov, float aspect, float aperture, float focusDist,
        float t0, float t1);

    inline Ray GetRay(float s, float t) const
    {
        Vec3  rd     = LensRadius * RandomInUnitDisk();
        Vec3  offset = U * rd.X() + V * rd.Y();
        float time = Time0 + RandomFloat()*(Time1 - Time0);

        return Ray(Origin + offset, LowerLeftCorner + (s * Horizontal) + (t * Vertical) - Origin - offset, time);
    }

private:

    Vec3   Origin;
    Vec3   LowerLeftCorner;
    Vec3   Horizontal;
    Vec3   Vertical;
    Vec3   U, V, W;
    float  Time0, Time1;
    float  LensRadius;
};