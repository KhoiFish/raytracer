
// ----------------------------------------------------------------------------------------------------------------------------
// Structs for input data

struct PixelShaderInput
{
    float4 PositionVS : POSITION0;
    float3 NormalVS   : NORMAL0;
    float2 TexCoord   : TEXCOORD;
};

struct Material
{
    float4 Emissive;
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float  SpecularPower;
    float3 Padding;
};

struct SpotLight
{
    float4 PositionWS;
    float4 PositionVS;
    float4 DirectionWS;
    float4 DirectionVS;
    float4 Color;
    float  SpotAngle;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    float  QuadraticAttenuation;
};

struct GlobalLightData
{
    int NumSpotLights;
};


// ----------------------------------------------------------------------------------------------------------------------------
// Structs used locally in this shader

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
};


// ----------------------------------------------------------------------------------------------------------------------------
// Lighting methods

float computeAttenuation(float c, float l, float q, float d)
{
    return 1.0f / (c + l * d + q * d * d);
}

float computeDiffuse(float3 normal, float3 lightDir)
{
    return max(0, dot(normal, lightDir));
}

float computeSpecular(float3 v, float3 normal, float3 lightDir, float specPower)
{
    float3 r        = normalize(reflect(-lightDir, normal));
    float  rDotView = max(0, dot(r, v));

    return pow(rDotView, specPower);
}

float computeSpotCone(float3 spotDir, float3 lightDir, float spotAngle)
{
    float  minCos   = cos(spotAngle);
    float  maxCos   = (minCos + 1.0f) / 2.0f;
    float  cosAngle = dot(spotDir, -lightDir);

    return smoothstep(minCos, maxCos, cosAngle);
}

LightResult computeSpotLight(SpotLight light, float3 v, float3 pos, float3 normal, float specPower)
{
    float3 lightDir       = (light.PositionVS.xyz - pos);
    float  lightDirLength = length(lightDir);

    lightDir = lightDir / lightDirLength;

    float  attenuation    = computeAttenuation(light.ConstantAttenuation, light.LinearAttenuation, light.QuadraticAttenuation, lightDirLength);
    float  spotIntensity  = computeSpotCone(light.DirectionVS.xyz, lightDir, light.SpotAngle);
    float4 finalIntensity = attenuation * spotIntensity * light.Color;

    LightResult result;
    result.Diffuse  = computeDiffuse(normal, lightDir) * finalIntensity;
    result.Specular = computeSpecular(v, normal, lightDir, specPower) * finalIntensity;

    return result;
}


// ----------------------------------------------------------------------------------------------------------------------------
// Input registers

SamplerState                      LinearRepeatSampler    : register(s0);
ConstantBuffer<Material>          MaterialCB             : register(b0, space1);
ConstantBuffer<GlobalLightData>   GlobalLightDataCB      : register(b1);
Texture2D                         DiffuseTexture         : register(t0);
StructuredBuffer<SpotLight>       SpotLights             : register(t1);


// ----------------------------------------------------------------------------------------------------------------------------
// Main entry

float4 main(PixelShaderInput IN) : SV_Target
{
    // Loop through the lights and compute the lighting result
    LightResult lightResult = (LightResult)0;
    {
        int    i      = 0;
        float3 posVS  = IN.PositionVS.xyz;
        float3 v      = normalize(-posVS);
        float3 normVS = normalize(IN.NormalVS);

        // Spot lights
        for (i = 0; i < GlobalLightDataCB.NumSpotLights; i++)
        {
            LightResult spotResult = computeSpotLight(SpotLights[i], v, posVS, normVS, MaterialCB.SpecularPower);
            lightResult.Diffuse  += spotResult.Diffuse;
            lightResult.Specular += spotResult.Specular;
        }
    }

    // Compute the final color
    float4 emissive   = MaterialCB.Emissive;
    float4 ambient    = MaterialCB.Ambient;
    float4 diffuse    = MaterialCB.Diffuse * lightResult.Diffuse;
    float4 specular   = MaterialCB.Specular * lightResult.Specular;
    float4 texColor   = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);
    float4 finalColor = (emissive + ambient + diffuse + specular) * texColor;

    return finalColor;
}
