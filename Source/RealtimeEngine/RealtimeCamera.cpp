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

#include "RealtimeCamera.h"

using namespace DirectX;
using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeCamera::RealtimeCamera()
    : ViewDirty( true )
    , InverseViewDirty( true )
    , ProjectionDirty( true )
    , InverseProjectionDirty( true )
    , VertFov( 45.0f )
    , AspectRatio( 1.0f )
    , ZNear( 0.1f )
    , ZFar( 100.0f )
{
    ProjectionJitter[0] = ProjectionJitter[1] = 0.0f;

    TheCameraData = new CameraData();
    TheCameraData->Translation = XMVectorZero();
    TheCameraData->Rotation    = XMQuaternionIdentity();
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeCamera::~RealtimeCamera()
{
    delete TheCameraData;
    TheCameraData = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetLookAt(FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up)
{
    TheCameraData->ViewMatrix  = XMMatrixLookAtRH(eye, target, up);
    TheCameraData->Translation = eye;
    TheCameraData->Rotation    = XMQuaternionRotationMatrix(XMMatrixTranspose(TheCameraData->ViewMatrix));
    TheCameraData->Eye         = eye;
    TheCameraData->Target      = target;

    InverseViewDirty = true;
    ViewDirty        = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RealtimeCamera::GetViewMatrix()
{
    if (ViewDirty)
    {
        UpdateViewMatrix();
    }

    return TheCameraData->ViewMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RealtimeCamera::GetInverseViewMatrix()
{
    if (InverseViewDirty)
    {
        TheCameraData->InverseViewMatrix = XMMatrixInverse(nullptr, TheCameraData->ViewMatrix);
        InverseViewDirty = false;
    }

    return TheCameraData->InverseViewMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetProjection(float fovy, float aspect, float zNear, float zFar)
{
    VertFov                = fovy;
    AspectRatio            = aspect;
    ZNear                  = zNear;
    ZFar                   = zFar;
    ProjectionDirty        = true;
    InverseProjectionDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RealtimeCamera::GetProjectionMatrix()
{
    if (ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    return TheCameraData->ProjectionMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RealtimeCamera::GetInverseProjectionMatrix()
{
    if (InverseProjectionDirty)
    {
        UpdateInverseProjectionMatrix();
    }

    return TheCameraData->InverseProjectionMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetVertFov(float fovy)
{
    if (VertFov != fovy)
    {
        VertFov                = fovy;
        ProjectionDirty        = true;
        InverseProjectionDirty = true;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

float RealtimeCamera::GetVertFov()
{
    return VertFov;
}

// ----------------------------------------------------------------------------------------------------------------------------

float RealtimeCamera::GetZFar()
{
    return ZFar;
}

// ----------------------------------------------------------------------------------------------------------------------------

float RealtimeCamera::GetZNear()
{
    return ZNear;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetTranslation(FXMVECTOR translation)
{
    TheCameraData->Translation = translation;
    ViewDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RealtimeCamera::GetTranslation()
{
    return TheCameraData->Translation;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetRotation(FXMVECTOR rotation)
{
    TheCameraData->Rotation = rotation;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::SetJitter(float jitterX, float jitterY)
{
    ProjectionJitter[0] = jitterX;
    ProjectionJitter[1] = jitterY;

    ProjectionDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RealtimeCamera::GetRotation()
{
    return TheCameraData->Rotation;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RealtimeCamera::GetEye()
{
    return TheCameraData->Eye;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RealtimeCamera::GetTarget()
{
    return TheCameraData->Target;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMFLOAT2 RealtimeCamera::GetJitter()
{
    return XMFLOAT2(ProjectionJitter[0], ProjectionJitter[1]);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::UpdateViewMatrix()
{
    XMMATRIX rotationMatrix    = XMMatrixTranspose(XMMatrixRotationQuaternion(TheCameraData->Rotation));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(-(TheCameraData->Translation));

    TheCameraData->ViewMatrix = translationMatrix * rotationMatrix;

    InverseViewDirty = true;
    ViewDirty        = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::UpdateInverseViewMatrix()
{
    if (ViewDirty)
    {
        UpdateViewMatrix();
    }

    TheCameraData->InverseViewMatrix = XMMatrixInverse(nullptr, TheCameraData->ViewMatrix);
    InverseViewDirty = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::UpdateProjectionMatrix()
{
    XMMATRIX projMat = XMMatrixPerspectiveFovRH(XMConvertToRadians(VertFov), AspectRatio, ZNear, ZFar);

    // Jitter
    float j[2] = { 2.0f * ProjectionJitter[0], 2.0f * ProjectionJitter[1] };
    XMMATRIX jitterMat
    (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        j[0], j[1], 0.0f, 1.0f
    );

    TheCameraData->ProjectionMatrix = projMat * jitterMat;

    ProjectionDirty        = false;
    InverseProjectionDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeCamera::UpdateInverseProjectionMatrix()
{
    if (ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    TheCameraData->InverseProjectionMatrix = XMMatrixInverse(nullptr, TheCameraData->ProjectionMatrix);
    InverseProjectionDirty = false;
}

