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
    float4 DepthPosition : TEXCOORD;
    float4 Position      : SV_Position;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<RenderMatrices> MatricesCB : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position      = mul(MatricesCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
    OUT.DepthPosition = OUT.Position;

    return OUT;
}
