#include "ShaderStructs.hlsl"
#include "Lighting.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionVS : POSITION0;
    float3 NormalVS   : NORMAL0;
    float2 TexCoord   : TEXCOORD;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState                      LinearRepeatSampler    : register(s0);
ConstantBuffer<Material>          MaterialCB             : register(b0, space1);
ConstantBuffer<GlobalLightData>   GlobalLightDataCB      : register(b1);
Texture2D                         DiffuseTexture         : register(t0);
StructuredBuffer<SpotLight>       SpotLights             : register(t1);

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    // Get the lighting result
    float4 lightingColor = computeLighting(
        IN.PositionVS.xyz, IN.NormalVS.xyz, 
        SpotLights, GlobalLightDataCB.NumSpotLights, 
        MaterialCB.Emissive, MaterialCB.Ambient, MaterialCB.Diffuse, MaterialCB.Specular, MaterialCB.SpecularPower);

    // Compute the final color
    float4 texColor   = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    float4 finalColor = lightingColor * texColor;

    return finalColor;
}
