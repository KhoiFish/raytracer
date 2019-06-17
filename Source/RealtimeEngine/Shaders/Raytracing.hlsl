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

#define HLSL
#include "../RealtimeSceneShaderInclude.h"
#include "../Renderer/RendererShaderInclude.h"
#include "../Renderer/RaytracingShaderInclude.h"
#include "RaytracingHelpers.hlsli"

// ----------------------------------------------------------------------------------------------------------------------------

#define SHADER_FLOAT_MAX    1.0e38f
#define SHADER_PI           3.141592f

// ----------------------------------------------------------------------------------------------------------------------------

// Global root signature
RWTexture2D<float4>                         gDirectResult         : register(u0, space0);
RWTexture2D<float4>                         gDirectAlbedo         : register(u1, space0);
RWTexture2D<float4>                         gIndirectResult       : register(u2, space0);
RWTexture2D<float4>                         gIndirectAlbedo       : register(u3, space0);

ConstantBuffer<RaytracingGlobalCB>          gSceneCB              : register(b0, space0);
ConstantBuffer<RealtimeAreaLight>           gLightsCB[]           : register(b0, space1);
ConstantBuffer<RenderMaterial>              gMaterialsCB[]        : register(b0, space2);

RaytracingAccelerationStructure             gScene                : register(t0, space0);
Texture2D<float4>                           gPositions            : register(t1, space0);
Texture2D<float4>                           gNormals              : register(t2, space0);
Texture2D<float4>                           gTexCoordsAndDepth    : register(t3, space0);
Texture2D<float4>                           gAlbedo               : register(t4, space0);

Texture2D<float4>                           gAlbedoArray[]        : register(t0, space1);
ByteAddressBuffer                           gVertexBufferArray[]  : register(t0, space2);
ByteAddressBuffer                           gIndexBufferArray[]   : register(t0, space3);

SamplerState                                AnisoRepeatSampler    : register(s0, space0);

// Local root signature
ConstantBuffer<RenderNodeInstanceData>      gInstanceDataCB       : register(b0, space4);
ConstantBuffer<RenderMaterial>              gMaterial             : register(b1, space4);

ByteAddressBuffer                           gVertexBuffer         : register(t0, space4);
ByteAddressBuffer                           gIndexBuffer          : register(t1, space4);

// ----------------------------------------------------------------------------------------------------------------------------

inline float shootAmbientOcclusionRay(float minT, float maxT, float3 origin, float3 dir)
{
    ShadowRayPayload payload;
    payload.Value = 0.0f;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = dir;
    ray.TMin      = minT;
    ray.TMax      = maxT;

    // Trace our ray.  Ray stops after it's first definite hit; never execute closest hit shader
    TraceRay
    (
        gScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.AOHitGroupIndex, gSceneCB.NumHitPrograms, gSceneCB.AOMissIndex,
        ray, payload
    );

    return payload.Value;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline AreaLightRayPayload shootAreaLightVisibilityRay(float3 orig, float3 dir, float minT, float maxT)
{
    AreaLightRayPayload payload;
    payload.LightIndex = -1;
    payload.LightColor = float3(0, 0, 0);

    RayDesc ray;
    ray.Origin    = orig;
    ray.Direction = dir;
    ray.TMin      = minT;
    ray.TMax      = maxT;

    TraceRay
    (
        gScene,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        RAYTRACING_INSTANCEMASK_ALL,
        gSceneCB.AreaLightHitGroupIndex, gSceneCB.NumHitPrograms, gSceneCB.AreaLightMissIndex,
        ray, payload
    );

    return payload;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shootShadeRay(uint instanceInclusionMask, inout uint randSeed, inout HaltonState state, float minT, float3 origin, float3 dir, uint curRayDepth)
{
    ShadeRayPayload payload;
    payload.RndSeed        = randSeed;
    payload.RayDepth       = curRayDepth + 1;
    payload.HState         = state;
    payload.Results.Result = float3(0, 0, 0); 
    payload.Results.Albedo = float3(0, 0, 0);

    // Shoot ray within the max ray depth
    if (payload.RayDepth <= gSceneCB.MaxRayDepth)
    {
        RayDesc ray;
        ray.Origin    = origin;
        ray.Direction = dir;
        ray.TMin      = minT;
        ray.TMax      = SHADER_FLOAT_MAX;

        TraceRay
        (
            gScene,
            RAY_FLAG_NONE,
            instanceInclusionMask,
            gSceneCB.ShadeHitGroupIndex, gSceneCB.NumHitPrograms, gSceneCB.ShadeMissIndex,
            ray, payload
        );

        randSeed = payload.RndSeed;
        state    = payload.HState;
    }

    return payload.Results;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shootIndirectLightRay(inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, uint curRayDepth)
{
    float3 bounceDir = getCosHemisphereSample(randSeed, state, worldNorm);
    return shootShadeRay(RAYTRACING_INSTANCEMASK_OPAQUE, randSeed, state, minT, worldPos, bounceDir, curRayDepth);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 getRandomPointOnAreaLight(inout uint randSeed, inout HaltonState state, RealtimeAreaLight light)
{
    float A0 = light.PlaneA0, A1 = light.PlaneA1;
    float B0 = light.PlaneB0, B1 = light.PlaneB1;
    float C0 = light.PlaneC0, C1 = light.PlaneC1;

    float a  = A0 + nextHaltonRand(randSeed, state) * (A1 - A0);
    float b  = B0 + nextHaltonRand(randSeed, state) * (B1 - B0);
    float c  = C0 + nextHaltonRand(randSeed, state) * (C1 - C0);

    return float3(a, b, c);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shadeLambert(RenderMaterial material, int raysPP, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    LightingResult lightResult;
    lightResult.Result = float3(0, 0, 0);
    lightResult.Albedo = float3(0, 0, 0);
    for (int i = 0; i < raysPP; i++)
    {
        // Pick a random light to sample
        int               lightIndex  = min(int(nextRand(randSeed) * gSceneCB.NumLights), gSceneCB.NumLights - 1);
        RealtimeAreaLight light       = gLightsCB[lightIndex];
        float3            onLight     = getRandomPointOnAreaLight(randSeed, state, light);
        float3            lightDir    = normalize(onLight - worldPos);
        float             dist        = length(onLight - worldPos);

        // Shoot a ray at the light
        AreaLightRayPayload lightPayload = shootAreaLightVisibilityRay(worldPos, lightDir, minT, SHADER_FLOAT_MAX);
        if (lightPayload.LightIndex >= 0)
        {
            float area            = light.AreaCoverage;
            float len             = dist;
            float distanceSquared = len * len;
            float cosine          = saturate(dot(lightDir, worldNorm));
            float pdf             = distanceSquared / (cosine * area);

            if (cosine > 0 && dot(lightDir, worldNorm) >= 0)
            {
                lightResult.Result += lightPayload.LightColor / pdf;
            }
        }
    }
    lightResult.Result /= raysPP;

    // Modulate albedo based on the physically based Lambertian term (albedo/pi)
    lightResult.Albedo = (albedo.rgb / SHADER_PI);

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shadeMetal(RenderMaterial material, float3 incomingRayDir, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, float4 albedo, uint curRayDepth)
{
    LightingResult lightResult;
    lightResult.Result = float3(0, 0, 0);
    lightResult.Albedo = float3(0, 0, 0);

    float3 reflDir = reflect(incomingRayDir, worldNorm) + (material.Fuzz * getCosHemisphereSample(randSeed, state, worldNorm));

    if (dot(reflDir, worldNorm) > 0.0f)
    {
        lightResult         = shootShadeRay(RAYTRACING_INSTANCEMASK_ALL, randSeed, state, minT, worldPos, reflDir, curRayDepth);

        // All the reflected results should go to the illumination channel for denoising
        lightResult.Result *= lightResult.Albedo;
        lightResult.Albedo  = albedo.rgb;
    }

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shadeDielectric(RenderMaterial material, float3 incomingRayDir, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, float4 albedo, uint curRayDepth)
{
    float  niOverNt, cosine;
    float3 outwardNormal;
    float  dirDotNorm = dot(incomingRayDir, worldNorm);
    if (dirDotNorm > 0.0f)
    {
        // Exiting triangle
        outwardNormal = -worldNorm;
        niOverNt      = material.ReflIndex;
        cosine        = material.ReflIndex * dirDotNorm;
    }
    else
    {
        // Entering triangle
        outwardNormal = worldNorm;
        niOverNt      = 1.0f / material.ReflIndex;
        cosine        = -dirDotNorm;
    }

    // Compute refract probability
    float3 refracted = refract(incomingRayDir, outwardNormal, niOverNt);
    float  reflectProb;
    if (any(refracted))
    {
        reflectProb = schlick(cosine, material.ReflIndex);
    }
    else
    {
        reflectProb = 1.0f;
    }

    // Choose randomly (w/ probability) whether to reflect or refract
    float3 specularDir;
    if (nextRand(randSeed) < reflectProb)
    {
        specularDir = reflect(incomingRayDir, worldNorm);
    }
    else
    {
        specularDir = refracted;
    }

    LightingResult lightResult = shootShadeRay(RAYTRACING_INSTANCEMASK_ALL, randSeed, state, minT, worldPos, specularDir, curRayDepth);

    // All the reflected results should go to the illumination channel for denoising
    lightResult.Result *= lightResult.Albedo;
    lightResult.Albedo  = albedo.rgb;

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult shade(RenderMaterial material, float3 incomingRayDir, int raysPP, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, float4 albedo, uint curRayDepth)
{
    LightingResult result;
    switch (material.Type)
    {
        case RenderMaterialType_Light:
            result.Albedo = float3(1, 1, 1);
            result.Result = material.Diffuse.rgb;
        break;

        case RenderMaterialType_Lambert:
            result = shadeLambert(material, raysPP, randSeed, state, minT, worldPos, worldNorm, albedo);
        break;

        case RenderMaterialType_Metal:
            result = shadeMetal(material, incomingRayDir, randSeed, state, minT, worldPos, worldNorm, albedo, curRayDepth);
        break;

        case RenderMaterialType_MDielectric:
            result = shadeDielectric(material, incomingRayDir, randSeed, state, minT, worldPos, worldNorm, albedo, curRayDepth);
        break;
    }

    return result;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float sampleAmbientOcclusion(int raysPP, inout uint randSeed, inout HaltonState state, float minT, float maxT, float3 worldPos, float3 worldNorm)
{
    float ao = 0.0f;
    for (int i = 0; i < raysPP; i++)
    {
        float3 worldDir = getCosHemisphereSample(randSeed, state, worldNorm);
        ao += shootAmbientOcclusionRay(minT, maxT, worldPos, worldDir);
    }
    ao = ao / float(raysPP);

    return ao;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult sampleDirectLighting(RenderMaterial material, float3 incomingRayDir, int raysPP, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    LightingResult lightResult;
    if (gSceneCB.NumLights > 0)
    {
        lightResult = shade(material, incomingRayDir, raysPP, randSeed, state, minT, worldPos, worldNorm, albedo, 0);
    }
    else
    {
        lightResult.Result = float3(1, 1, 1);
        lightResult.Albedo = (albedo.rgb / raysPP);
    }

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline LightingResult sampleIndirectLighting(int raysPP, inout uint randSeed, inout HaltonState state, float minT, float3 worldPos, float3 worldNorm)
{
    LightingResult lightResult;
    lightResult.Result = float3(0, 0, 0);
    lightResult.Albedo = float3(0, 0, 0);

    if (gSceneCB.NumLights > 0)
    {
        for (int i = 0; i < raysPP; i++)
        {
            LightingResult result = shootIndirectLightRay(randSeed, state, minT, worldPos, worldNorm, 0);
            lightResult.Result += result.Result;
            lightResult.Albedo += result.Albedo;
        }
        lightResult.Result /= raysPP;
        lightResult.Albedo /= raysPP;
    }

    // All the reflected results should go to the illumination channel for denoising
    lightResult.Result *= lightResult.Albedo;
    lightResult.Albedo = float3(1, 1, 1);

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void AOMiss(inout ShadowRayPayload payload)
{
    // Our ambient occlusion value is 1 if we hit nothing
    payload.Value = 1.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void AOClosest(inout ShadowRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void AreaLightMiss(inout AreaLightRayPayload payload)
{
    // Missed the light
    payload.LightIndex = -1;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void AreaLightClosest(inout AreaLightRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    // Hit the light
    payload.LightIndex = gInstanceDataCB.LightIndex;
    payload.LightColor = gMaterial.Diffuse.rgb;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void ShadeMiss(inout ShadeRayPayload payload)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void ShadeClosest(inout ShadeRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    // Hit an opaque
    float3               bary       = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    RealtimeSceneVertex  vert       = getVertex(gIndexBuffer, gVertexBuffer, PrimitiveIndex(), bary);
    float3               rayOrig    = WorldRayOrigin();
    float3               rayDir     = WorldRayDirection();
    float3               worldPos   = rayOrig + RayTCurrent() * rayDir;
    float3               worldNorm  = mul(vert.Normal, (float3x3)gInstanceDataCB.WorldMatrix);
    float4               albedo     = gAlbedoArray[gMaterial.DiffuseTextureId].SampleLevel(AnisoRepeatSampler, vert.TexCoord, 0);

    // Shade
    payload.Results = shade(gMaterial, WorldRayDirection(), gSceneCB.RaysPerPixel, payload.RndSeed, payload.HState, RayTMin(), worldPos, worldNorm, albedo, payload.RayDepth);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline void validateLightingResult(inout LightingResult lightResult)
{
    lightResult.Result = any(isnan(lightResult.Result)) ? float3(0, 0, 0) : lightResult.Result;
    lightResult.Albedo = any(isnan(lightResult.Albedo)) ? float3(0, 0, 0) : lightResult.Albedo;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("raygeneration")]
void RayGeneration()
{
    // Inputs
    int             raysPP      = gSceneCB.RaysPerPixel;
    uint3           launchIndex = DispatchRaysIndex();
    uint3           launchDim   = DispatchRaysDimensions();
    uint            randSeed    = initRand(launchIndex.x + launchIndex.y * launchDim.x, gSceneCB.FrameCount, 16);
    HaltonState     haltonState = haltonInit(launchIndex.x, launchIndex.y, 0, 16, gSceneCB.FrameCount, 64, 16);
    float4          worldPos    = gPositions[launchIndex.xy];
    float3          rayInDir    = normalize(worldPos.xyz - gSceneCB.CameraPosition.xyz);
    float4          normAndId   = gNormals[launchIndex.xy];
    float3          worldNorm   = normAndId.xyz;
    int             instanceId  = int(normAndId.w);
    RenderMaterial  material    = gMaterialsCB[instanceId];
    float4          albedo      = gAlbedo[launchIndex.xy];
    float           aoRadius    = gSceneCB.AORadius;
    float           minT        = 0.001f;

    // Get direct, indirect and AO contributions
    LightingResult directResult   = sampleDirectLighting(material, rayInDir, raysPP, randSeed, haltonState, minT, worldPos.xyz, worldNorm, albedo);
    LightingResult indirectResult = sampleIndirectLighting(raysPP, randSeed, haltonState, minT, worldPos.xyz, worldNorm);

    // Zero out nan, inf, etc.
    validateLightingResult(directResult);
    validateLightingResult(indirectResult);

    // Write final results to the output
    gDirectResult[launchIndex.xy]   = float4(directResult.Result, 1);
    gDirectAlbedo[launchIndex.xy]   = float4(directResult.Albedo, 1);
    gIndirectResult[launchIndex.xy] = float4(indirectResult.Result, 1);
    gIndirectAlbedo[launchIndex.xy] = float4(indirectResult.Albedo, 1);
}
