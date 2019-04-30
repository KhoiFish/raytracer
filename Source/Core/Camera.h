// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#pragma once

#include "Ray.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
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
            Vec4  rd = LensRadius * RandomInUnitDisk();
            Vec4  offset = U * rd.X() + V * rd.Y();
            float time = Time0 + RandomFloat() * (Time1 - Time0);

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
            Vec4 & lookFrom, Vec4 & lookAt, Vec4 & up,
            float& vertFov, float& aspect, float& aperture, float& focusDist,
            float& t0, float& t1, Vec4 & clearColor) const
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
}