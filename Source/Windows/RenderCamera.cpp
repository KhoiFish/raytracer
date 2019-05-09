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

RenderCamera::RenderCamera()
    : m_ViewDirty( true )
    , m_InverseViewDirty( true )
    , m_ProjectionDirty( true )
    , m_InverseProjectionDirty( true )
    , m_vFoV( 45.0f )
    , m_AspectRatio( 1.0f )
    , m_zNear( 0.1f )
    , m_zFar( 100.0f )
{
    pData = (AlignedData*)_aligned_malloc( sizeof(AlignedData), 16 );
    pData->m_Translation = XMVectorZero();
    pData->m_Rotation = XMQuaternionIdentity();
}

RenderCamera::~RenderCamera()
{
    _aligned_free(pData);
}

void XM_CALLCONV RenderCamera::set_LookAt( FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up )
{
    pData->m_ViewMatrix = XMMatrixLookAtLH( eye, target, up );

    pData->m_Translation = eye;
    pData->m_Rotation = XMQuaternionRotationMatrix( XMMatrixTranspose(pData->m_ViewMatrix) );

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

XMMATRIX RenderCamera::get_ViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

XMMATRIX RenderCamera::get_InverseViewMatrix() const
{
    if ( m_InverseViewDirty )
    {
        pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
        m_InverseViewDirty = false;
    }

    return pData->m_InverseViewMatrix;
}

void RenderCamera::set_Projection( float fovy, float aspect, float zNear, float zFar )
{
    m_vFoV = fovy;
    m_AspectRatio = aspect;
    m_zNear = zNear;
    m_zFar = zFar;

    m_ProjectionDirty = true;
    m_InverseProjectionDirty = true;
}

XMMATRIX RenderCamera::get_ProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

XMMATRIX RenderCamera::get_InverseProjectionMatrix() const
{
    if ( m_InverseProjectionDirty )
    {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void RenderCamera::set_FoV(float fovy)
{
    if (m_vFoV != fovy)
    {
        m_vFoV = fovy;
        m_ProjectionDirty = true;
        m_InverseProjectionDirty = true;
    }
}

float RenderCamera::get_FoV() const
{
    return m_vFoV;
}


void XM_CALLCONV RenderCamera::set_Translation( FXMVECTOR translation )
{
    pData->m_Translation = translation;
    m_ViewDirty = true;
}

XMVECTOR RenderCamera::get_Translation() const
{
    return pData->m_Translation;
}

void RenderCamera::set_Rotation( FXMVECTOR rotation )
{
    pData->m_Rotation = rotation;
}

XMVECTOR RenderCamera::get_Rotation() const
{
    return pData->m_Rotation;
}

void XM_CALLCONV RenderCamera::Translate( FXMVECTOR translation, Space space )
{
    switch ( space )
    {
    case Space::Local:
        {
            pData->m_Translation += XMVector3Rotate( translation, pData->m_Rotation );
        }
        break;
    case Space::World:
        {
            pData->m_Translation += translation;
        }
        break;
    }

    pData->m_Translation = XMVectorSetW( pData->m_Translation, 1.0f );

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void RenderCamera::Rotate( FXMVECTOR quaternion )
{
    pData->m_Rotation = XMQuaternionMultiply( pData->m_Rotation, quaternion );

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void RenderCamera::UpdateViewMatrix() const
{
    XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion( pData->m_Rotation ));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( -(pData->m_Translation) );

    pData->m_ViewMatrix = translationMatrix * rotationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

void RenderCamera::UpdateInverseViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
    m_InverseViewDirty = false;
}

void RenderCamera::UpdateProjectionMatrix() const
{
    pData->m_ProjectionMatrix = XMMatrixPerspectiveFovLH( XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar );

    m_ProjectionDirty = false;
    m_InverseProjectionDirty = true;
}

void RenderCamera::UpdateInverseProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    pData->m_InverseProjectionMatrix = XMMatrixInverse( nullptr, pData->m_ProjectionMatrix );
    m_InverseProjectionDirty = false;
}
