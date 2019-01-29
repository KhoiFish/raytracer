#include "ShaderStructs.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionVS : POSITION0;
    float3 NormalVS   : NORMAL0;
    float2 TexCoord   : TEXCOORD0;
    float4 SMCoord    : TEXCOORD1;
    float4 Position   : SV_Position;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<RenderMatrices> MatricesCB : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    float4 worldPos = mul(float4(IN.Position, 1.0f), MatricesCB.ModelMatrix);

    OUT.Position   = mul(float4(IN.Position, 1.0f), MatricesCB.ModelViewProjectionMatrix);
    OUT.PositionVS = mul(float4(IN.Position, 1.0f), MatricesCB.ModelViewMatrix);
    OUT.NormalVS   = mul(IN.Normal, (float3x3)MatricesCB.InverseTransposeModelViewMatrix);
    OUT.TexCoord   = IN.TexCoord;
    OUT.SMCoord    = mul(worldPos, MatricesCB.ShadowViewProj);

    return OUT;
}
