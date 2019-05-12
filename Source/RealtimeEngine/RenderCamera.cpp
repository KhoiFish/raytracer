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

#include "RenderCamera.h"

using namespace DirectX;

// ----------------------------------------------------------------------------------------------------------------------------

RenderCamera::RenderCamera()
    : ViewDirty( true )
    , InverseViewDirty( true )
    , ProjectionDirty( true )
    , InverseProjectionDirty( true )
    , VertFov( 45.0f )
    , AspectRatio( 1.0f )
    , ZNear( 0.1f )
    , ZFar( 100.0f )
{
    TheCameraData = new CameraData();
    TheCameraData->Translation = XMVectorZero();
    TheCameraData->Rotation    = XMQuaternionIdentity();
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderCamera::~RenderCamera()
{
    delete TheCameraData;
    TheCameraData = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::SetLookAt(FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up)
{
    TheCameraData->ViewMatrix  = XMMatrixLookAtRH( eye, target, up );
    TheCameraData->Translation = eye;
    TheCameraData->Rotation    = XMQuaternionRotationMatrix(XMMatrixTranspose(TheCameraData->ViewMatrix));

    InverseViewDirty = true;
    ViewDirty        = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RenderCamera::GetViewMatrix()
{
    if (ViewDirty)
    {
        UpdateViewMatrix();
    }

    return TheCameraData->ViewMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RenderCamera::GetInverseViewMatrix()
{
    if (InverseViewDirty)
    {
        TheCameraData->InverseViewMatrix = XMMatrixInverse(nullptr, TheCameraData->ViewMatrix);
        InverseViewDirty = false;
    }

    return TheCameraData->InverseViewMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::SetProjection(float fovy, float aspect, float zNear, float zFar)
{
    VertFov                = fovy;
    AspectRatio            = aspect;
    ZNear                  = zNear;
    ZFar                   = zFar;
    ProjectionDirty        = true;
    InverseProjectionDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RenderCamera::GetProjectionMatrix()
{
    if (ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    return TheCameraData->ProjectionMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMMATRIX RenderCamera::GetInverseProjectionMatrix()
{
    if (InverseProjectionDirty)
    {
        UpdateInverseProjectionMatrix();
    }

    return TheCameraData->InverseProjectionMatrix;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::SetVertFov(float fovy)
{
    if (VertFov != fovy)
    {
        VertFov                = fovy;
        ProjectionDirty        = true;
        InverseProjectionDirty = true;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

float RenderCamera::GetVertFov()
{
    return VertFov;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::SetTranslation(FXMVECTOR translation)
{
    TheCameraData->Translation = translation;
    ViewDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RenderCamera::GetTranslation()
{
    return TheCameraData->Translation;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::SetRotation(FXMVECTOR rotation)
{
    TheCameraData->Rotation = rotation;
}

// ----------------------------------------------------------------------------------------------------------------------------

XMVECTOR RenderCamera::GetRotation()
{
    return TheCameraData->Rotation;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::UpdateViewMatrix()
{
    XMMATRIX rotationMatrix    = XMMatrixTranspose(XMMatrixRotationQuaternion(TheCameraData->Rotation));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(-(TheCameraData->Translation));

    TheCameraData->ViewMatrix = translationMatrix * rotationMatrix;

    InverseViewDirty = true;
    ViewDirty        = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::UpdateInverseViewMatrix()
{
    if (ViewDirty)
    {
        UpdateViewMatrix();
    }

    TheCameraData->InverseViewMatrix = XMMatrixInverse(nullptr, TheCameraData->ViewMatrix);
    InverseViewDirty = false;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::UpdateProjectionMatrix()
{
    TheCameraData->ProjectionMatrix = XMMatrixPerspectiveFovRH(XMConvertToRadians(VertFov), AspectRatio, ZNear, ZFar);

    ProjectionDirty        = false;
    InverseProjectionDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderCamera::UpdateInverseProjectionMatrix()
{
    if (ProjectionDirty)
    {
        UpdateProjectionMatrix();
    }

    TheCameraData->InverseProjectionMatrix = XMMatrixInverse(nullptr, TheCameraData->ProjectionMatrix);
    InverseProjectionDirty = false;
}
