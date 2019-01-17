#include "CameraDX12.h"

using namespace DirectX;

CameraDX12::CameraDX12()
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

CameraDX12::~CameraDX12()
{
    _aligned_free(pData);
}

void XM_CALLCONV CameraDX12::set_LookAt( FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up )
{
    pData->m_ViewMatrix = XMMatrixLookAtLH( eye, target, up );

    pData->m_Translation = eye;
    pData->m_Rotation = XMQuaternionRotationMatrix( XMMatrixTranspose(pData->m_ViewMatrix) );

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

XMMATRIX CameraDX12::get_ViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

XMMATRIX CameraDX12::get_InverseViewMatrix() const
{
    if ( m_InverseViewDirty )
    {
        pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
        m_InverseViewDirty = false;
    }

    return pData->m_InverseViewMatrix;
}

void CameraDX12::set_Projection( float fovy, float aspect, float zNear, float zFar )
{
    m_vFoV = fovy;
    m_AspectRatio = aspect;
    m_zNear = zNear;
    m_zFar = zFar;

    m_ProjectionDirty = true;
    m_InverseProjectionDirty = true;
}

XMMATRIX CameraDX12::get_ProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

XMMATRIX CameraDX12::get_InverseProjectionMatrix() const
{
    if ( m_InverseProjectionDirty )
    {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void CameraDX12::set_FoV(float fovy)
{
    if (m_vFoV != fovy)
    {
        m_vFoV = fovy;
        m_ProjectionDirty = true;
        m_InverseProjectionDirty = true;
    }
}

float CameraDX12::get_FoV() const
{
    return m_vFoV;
}


void XM_CALLCONV CameraDX12::set_Translation( FXMVECTOR translation )
{
    pData->m_Translation = translation;
    m_ViewDirty = true;
}

XMVECTOR CameraDX12::get_Translation() const
{
    return pData->m_Translation;
}

void CameraDX12::set_Rotation( FXMVECTOR rotation )
{
    pData->m_Rotation = rotation;
}

XMVECTOR CameraDX12::get_Rotation() const
{
    return pData->m_Rotation;
}

void XM_CALLCONV CameraDX12::Translate( FXMVECTOR translation, Space space )
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

void CameraDX12::Rotate( FXMVECTOR quaternion )
{
    pData->m_Rotation = XMQuaternionMultiply( pData->m_Rotation, quaternion );

    m_ViewDirty = true;
    m_InverseViewDirty = true;
}

void CameraDX12::UpdateViewMatrix() const
{
    XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion( pData->m_Rotation ));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( -(pData->m_Translation) );

    pData->m_ViewMatrix = translationMatrix * rotationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

void CameraDX12::UpdateInverseViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
    m_InverseViewDirty = false;
}

void CameraDX12::UpdateProjectionMatrix() const
{
    pData->m_ProjectionMatrix = XMMatrixPerspectiveFovLH( XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar );

    m_ProjectionDirty = false;
    m_InverseProjectionDirty = true;
}

void CameraDX12::UpdateInverseProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    pData->m_InverseProjectionMatrix = XMMatrixInverse( nullptr, pData->m_ProjectionMatrix );
    m_InverseProjectionDirty = false;
}
