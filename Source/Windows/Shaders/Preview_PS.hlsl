struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

struct Material
{
    float4   Emissive;
    float4   Ambient;
    float4   Diffuse;
    float4   Specular;
    float    SpecularPower;
    float3   Padding;
};

Texture2D                  DiffuseTexture         : register(t0);
SamplerState               LinearRepeatSampler    : register(s0);
ConstantBuffer<Material>   MaterialCB             : register(b0, space1);


float4 main( PixelShaderInput IN ) : SV_Target
{
    float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);

    return texColor * (MaterialCB.Diffuse);
}