struct PixelShaderInput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

Texture2D      DiffuseTexture         : register(t0);
SamplerState   LinearRepeatSampler    : register(s0);


float4 main( PixelShaderInput IN ) : SV_Target
{
    float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);

    return texColor;
}