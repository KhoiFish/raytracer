// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

struct LightResult
{
    float4 Diffuse;
    float4 Specular;
};

// ----------------------------------------------------------------------------------------------------------------------------

float computeAttenuation(float c, float l, float q, float d)
{
    return 1.0f / (c + l * d + q * d * d);
}

// ----------------------------------------------------------------------------------------------------------------------------

float computeDiffuse(float3 normal, float3 lightDir)
{
    return max(0, dot(normal, lightDir));
}

// ----------------------------------------------------------------------------------------------------------------------------

float computeSpecular(float3 eye, float3 normal, float3 lightDir, float specPower)
{
    float3 r        = normalize(reflect(-lightDir, normal));
    float  rDotView = max(0, dot(r, eye));

    return pow(rDotView, specPower);
}

// ----------------------------------------------------------------------------------------------------------------------------

float computeSpotCone(float3 spotDir, float3 lightDir, float spotAngle)
{
    float minCos   = cos(spotAngle);
    float maxCos   = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(spotDir, -lightDir);

    return smoothstep(minCos, maxCos, cosAngle);
}

// ----------------------------------------------------------------------------------------------------------------------------

LightResult computeSpotLight(SpotLight light, float3 eye, float3 pos, float3 normal, float specPower)
{
    float3 lightDir       = (light.PositionVS.xyz - pos);
    float  lightDirLength = length(lightDir);

    lightDir = lightDir / lightDirLength;

    float  attenuation    = computeAttenuation(light.ConstantAttenuation, light.LinearAttenuation, light.QuadraticAttenuation, lightDirLength);
    float  spotIntensity  = computeSpotCone(light.DirectionVS.xyz, lightDir, light.SpotAngle);
    float4 finalIntensity = attenuation * spotIntensity * light.Color;

    LightResult result;
    result.Diffuse  = computeDiffuse(normal, lightDir) * finalIntensity;
    result.Specular = computeSpecular(eye, normal, lightDir, specPower) * finalIntensity;

    return result;
}

// ----------------------------------------------------------------------------------------------------------------------------

LightResult computeDirLight(DirLight light, float3 eye, float3 normal, float specPower)
{
    float3 lightDir       = light.DirectionVS.xyz;
    float  intensity      = max(dot(normal, lightDir), 0);
    float4 finalIntensity = intensity * light.Color;

    LightResult result;
    result.Diffuse  = computeDiffuse(normal, lightDir) * finalIntensity;
    result.Specular = computeSpecular(eye, normal, lightDir, specPower) * finalIntensity;

    return result;
}

// ----------------------------------------------------------------------------------------------------------------------------

float4 computeLighting(float3 posVS, float3 normVS, 
    StructuredBuffer<SpotLight> spotLights, int numSpotLights, 
    StructuredBuffer<DirLight> dirLights, int numDirLights,
    float4 emissive, float4 ambient, float4 diffuse, float4 specular, float specPower)
{
    LightResult lightResult = (LightResult)0;
    {
        int    i   = 0;
        float3 eye = normalize(-posVS);

        // Spot lights
        for (i = 0; i < numSpotLights; i++)
        {
            LightResult spotResult = computeSpotLight(spotLights[i], eye, posVS, normVS, specPower);
            lightResult.Diffuse  += spotResult.Diffuse;
            lightResult.Specular += spotResult.Specular;
        }

        // Dir lights
        for (i = 0; i < numDirLights; i++)
        {
            LightResult dirResult = computeDirLight(dirLights[i], eye, normVS, specPower);
            lightResult.Diffuse  += dirResult.Diffuse;
            lightResult.Specular += dirResult.Specular;
        }
    }

    float4 finalColor = emissive + ambient + (diffuse * lightResult.Diffuse) + (specular * lightResult.Specular);

    return finalColor;
}
