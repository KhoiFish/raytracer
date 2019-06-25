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

#include "Camera.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

Camera::Camera()
    : LookFrom(0, 0, 0)
    , LookAt(0, 0, 0)
    , Up(0, 0, 0)
    , VertFov(0)
    , Aspect(0)
    , Aperture(0)
    , FocusDist(0)
    , BackgroundColor(0, 0, 0)
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

Camera::Camera(Vec4 lookFrom, Vec4 lookAt, Vec4 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1, Vec4 backgroundColor)
{
    Setup(lookFrom, lookAt, vup, vertFov, aspect, aperture, focusDist, t0, t1, backgroundColor);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::Setup(Vec4 lookFrom, Vec4 lookAt, Vec4 vup, float vertFov, float aspect, float aperture, float focusDist, float t0, float t1, Vec4 backgroundColor)
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
    BackgroundColor  = backgroundColor;

    UpdateInternalSettings();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::SetAspect(float aspect)
{
    Aspect = aspect;
    UpdateInternalSettings();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Camera::SetFocusDistanceToLookAt()
{
    FocusDist = (LookAt - LookFrom).Length();
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
