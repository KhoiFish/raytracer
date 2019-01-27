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
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<RenderMatrices> MatricesCB : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position   = mul(MatricesCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.PositionVS = mul(MatricesCB.ModelViewMatrix, float4(IN.Position, 1.0f));
    OUT.NormalVS   = mul((float3x3)MatricesCB.InverseTransposeModelViewMatrix, IN.Normal);
    OUT.TexCoord   = IN.TexCoord;

    return OUT;
}
