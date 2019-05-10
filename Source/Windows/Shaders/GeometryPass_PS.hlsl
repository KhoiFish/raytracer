#include "ShaderStructs.hlsl"
#include "Lighting.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionVS : TEXCOORD0;
    float3 NormalVS   : TEXCOORD1;
    float2 TexCoord   : TEXCOORD2;
    float4 SMCoord    : TEXCOORD3;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct MRT
{
    float4 Position : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 TexCoord : SV_Target2;
    float4 Diffuse  : SV_Target3;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState LinearRepeatSampler    : register(s0);
Texture2D    DiffuseTexture         : register(t0);

// ----------------------------------------------------------------------------------------------------------------------------

MRT main(PixelShaderInput IN)
{
    MRT mrt;
    mrt.Position = IN.PositionVS;
    mrt.Normal   = float4(IN.NormalVS, 1);
    mrt.TexCoord = float4(IN.TexCoord, 0, 0);
    mrt.Diffuse  = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);

    return mrt;
}
