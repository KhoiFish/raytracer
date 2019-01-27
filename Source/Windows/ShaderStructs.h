#pragma once

#include <wrl.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <stdint.h>

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
    SpotLight()
        : PositionWS(0.0f, 0.0f, 0.0f, 1.0f)
        , PositionVS(0.0f, 0.0f, 0.0f, 1.0f)
        , DirectionWS(0.0f, 0.0f, 1.0f, 0.0f)
        , DirectionVS(0.0f, 0.0f, 1.0f, 0.0f)
        , Color(1.0f, 1.0f, 1.0f, 1.0f)
        , SpotAngle(DirectX::XM_PIDIV2)
        , ConstantAttenuation(1.0f)
        , LinearAttenuation(0.0f)
        , QuadraticAttenuation(0.0f)
    {}

    DirectX::XMFLOAT4    PositionWS;
    DirectX::XMFLOAT4    PositionVS;
    DirectX::XMFLOAT4    DirectionWS;
    DirectX::XMFLOAT4    DirectionVS;
    DirectX::XMFLOAT4    Color;
    float                SpotAngle;
    float                ConstantAttenuation;
    float                LinearAttenuation;
    float                QuadraticAttenuation;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------------------------------------------------------

struct GlobalLightData
{
    int NumSpotLights;
};
