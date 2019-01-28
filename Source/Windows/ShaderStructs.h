#pragma once

#include <wrl.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <stdint.h>

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 16)
struct RenderMatrices
{
    DirectX::XMMATRIX ModelMatrix;
    DirectX::XMMATRIX ViewMatrix;
    DirectX::XMMATRIX ProjectionMatrix;
    DirectX::XMMATRIX ModelViewMatrix;
    DirectX::XMMATRIX InverseTransposeModelViewMatrix;
    DirectX::XMMATRIX ModelViewProjectionMatrix;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 16)
struct RenderMaterial
{
    RenderMaterial(
        DirectX::XMFLOAT4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f },
        DirectX::XMFLOAT4 ambient = { 0.1f, 0.1f, 0.1f, 1.0f },
        DirectX::XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f },
        DirectX::XMFLOAT4 specular = { 1.0f, 1.0f, 1.0f, 1.0f },
        float specularPower = 128.0f
    )
        : Emissive(emissive)
        , Ambient(ambient)
        , Diffuse(diffuse)
        , Specular(specular)
        , SpecularPower(specularPower) {}

    DirectX::XMFLOAT4   Emissive;
    DirectX::XMFLOAT4   Ambient;
    DirectX::XMFLOAT4   Diffuse;
    DirectX::XMFLOAT4   Specular;
    float               SpecularPower;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 16)
struct SpotLight
{
    DirectX::XMFLOAT4    PositionWS;
    DirectX::XMFLOAT4    PositionVS;
    DirectX::XMFLOAT4    DirectionWS;
    DirectX::XMFLOAT4    DirectionVS;
    DirectX::XMFLOAT4    LookAtWS;
    DirectX::XMFLOAT4    UpWS;
    DirectX::XMFLOAT4    SmapWS;
    DirectX::XMFLOAT4    Color;
    float                SpotAngle;
    float                ConstantAttenuation;
    float                LinearAttenuation;
    float                QuadraticAttenuation;
    int                  ShadowmapId;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 16)
struct DirLight
{
    DirectX::XMFLOAT4    DirectionWS;
    DirectX::XMFLOAT4    DirectionVS;
    DirectX::XMFLOAT4    Color;
    int                  ShadowmapId;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

#pragma pack(push, 16)
struct GlobalData
{
    int   NumSpotLights;
    int   NumDirLights;
    float ShadowmapDepth;
};
#pragma pack(pop)