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

#include <DirectXMath.h>

class RenderCamera
{
public:

    RenderCamera();
    virtual ~RenderCamera();

    void                SetLookAt(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up);
    void                SetProjection(float fovy, float aspect, float zNear, float zFar);
    void                SetVertFov(float fovy);
    void                SetTranslation(DirectX::FXMVECTOR translation);
    void                SetRotation(DirectX::FXMVECTOR rotation);

    DirectX::XMMATRIX   GetViewMatrix();
    DirectX::XMMATRIX   GetInverseViewMatrix();
    DirectX::XMMATRIX   GetProjectionMatrix();
    DirectX::XMMATRIX   GetInverseProjectionMatrix();
    DirectX::XMVECTOR   GetTranslation();
    DirectX::XMVECTOR   GetRotation();
    DirectX::XMVECTOR   GetEye();
    DirectX::XMVECTOR   GetTarget();
    float               GetVertFov();

private:

    virtual void        UpdateViewMatrix();
    virtual void        UpdateInverseViewMatrix();
    virtual void        UpdateProjectionMatrix();
    virtual void        UpdateInverseProjectionMatrix();

private:

    struct CameraData
    {
        DirectX::XMVECTOR Eye;
        DirectX::XMVECTOR Target;
        DirectX::XMVECTOR Translation;
        DirectX::XMVECTOR Rotation;
        DirectX::XMMATRIX ViewMatrix;
        DirectX::XMMATRIX InverseViewMatrix;
        DirectX::XMMATRIX ProjectionMatrix;
        DirectX::XMMATRIX InverseProjectionMatrix;
    };

private:

    CameraData*     TheCameraData;
    float           VertFov;
    float           AspectRatio;
    float           ZNear;
    float           ZFar;
    bool            ViewDirty;
    bool            InverseViewDirty;
    bool            ProjectionDirty;
    bool            InverseProjectionDirty;

private:

};