#include "ShaderStructs.hlsl"
#include "Lighting.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionVS : POSITION0;
    float3 NormalVS   : NORMAL0;
    float2 TexCoord   : TEXCOORD0;
    float4 SMCoord    : TEXCOORD1;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState                      LinearRepeatSampler    : register(s0);
SamplerState                      LinearClampSampler     : register(s1);
ConstantBuffer<RenderMaterial>    MaterialCB             : register(b0);
ConstantBuffer<GlobalData>        GlobalDataCB           : register(b1);
Texture2D                         DiffuseTexture         : register(t0);
//Texture2D                         ShadowmapTexture       : register(t1);
//StructuredBuffer<SpotLight>       SpotLights             : register(t0, space1);
//StructuredBuffer<DirLight>        DirLights              : register(t1, space1);

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    //// Get the lighting result
    //float4 lightingColor = computeLighting(
    //    IN.PositionVS.xyz, IN.NormalVS.xyz, 
    //    SpotLights, GlobalDataCB.NumSpotLights,
    //    DirLights, GlobalDataCB.NumDirLights,
    //    MaterialCB.Emissive, MaterialCB.Ambient, MaterialCB.Diffuse, MaterialCB.Specular, MaterialCB.SpecularPower);

    //// See if we're in shadow
    //float  bias         = 0.005f;
    //float  visibility   = 1.0f;
    //float3 shadowCoord  = IN.SMCoord.xyz / IN.SMCoord.w;
    //float  curDepth     = shadowCoord.z - bias;
    //float  sampledDepth = ShadowmapTexture.Sample(LinearClampSampler, shadowCoord.xy).z;
    //if (sampledDepth < curDepth)
    //{
    //    visibility = 0.3f;
    //}

    // Compute the final color
    //float4 texColor   = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    //float4 finalColor = texColor; //lightingColor * texColor * visibility;

    float4 finalColor = float4(1, 0, 0, 1);


    return finalColor;
}
