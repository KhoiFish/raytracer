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
#include "../RealtimeEngine/RealtimeSceneShaderInclude.h"
#include "../RendererShaderInclude.h"
#include "../RaytracingShaderInclude.h"
#include "RaytracingHelpers.hlsli"

// ----------------------------------------------------------------------------------------------------------------------------

#define SHADER_FLOAT_MAX    1.0e38f
#define SHADER_PI           3.141592f

// ----------------------------------------------------------------------------------------------------------------------------

// Global root signature
RWTexture2D<float4>                         gPrevDirectAO       : register(u0, space0);
RWTexture2D<float4>                         gCurrDirectAO       : register(u1, space0);
RWTexture2D<float4>                         gPrevIndirect       : register(u2, space0);
RWTexture2D<float4>                         gCurrIndirect       : register(u3, space0);

ConstantBuffer<RaytracingGlobalCB>          gSceneCB            : register(b0, space0);
ConstantBuffer<RealtimeAreaLight>           gLightsCB[]         : register(b1, space0);

RaytracingAccelerationStructure             gScene              : register(t0, space0);
Texture2D<float4>                           gPositions          : register(t1, space0);
Texture2D<float4>                           gNormals            : register(t2, space0);
Texture2D<float4>                           gTexCoordsAndDepth  : register(t3, space0);
Texture2D<float4>                           gAlbedo             : register(t4, space0);
Texture2D<float4>                           gAlbedoArray[]      : register(t5, space0);


// Local root signature
ConstantBuffer<RenderNodeInstanceData>      gInstanceDataCB     : register(b0, space1);
ConstantBuffer<RenderMaterial>              gMaterial           : register(b1, space1);

ByteAddressBuffer                           gVertexBuffer       : register(t0, space1);
ByteAddressBuffer                           gIndexBuffer        : register(t1, space1);

// ----------------------------------------------------------------------------------------------------------------------------

inline uint3 getIndices(uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int  address   = (baseIndex * 4);

    return gIndexBuffer.Load3(address);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline RealtimeSceneVertex getVertex(uint triangleIndex, float3 barycentrics)
{
    RealtimeSceneVertex v;
    v.Position = float3(0, 0, 0);
    v.Normal   = float3(0, 0, 0);
    v.TexCoord = float2(0, 0);

    const uint vertSize = (3 * 4) + (3 * 4) + (2 * 4);
    uint3      indices  = getIndices(triangleIndex);
    for (uint i = 0; i < 3; i++)
    {
        int address = indices[i] * vertSize;

        v.Position += asfloat(gVertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.Normal += asfloat(gVertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.TexCoord += asfloat(gVertexBuffer.Load2(address)) * barycentrics[i];
    }

    return v;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline DirectRayPayload areaLightVisibility(float3 orig, float3 dir, float minT, float maxT)
{
    DirectRayPayload payload;
    payload.LightIndex     = -1;
    payload.HitBaryAndDist = float4(0, 0, 0, 0);
    payload.LightColor     = float3(0, 0, 0);

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
        gSceneCB.DirectLightingHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.DirectLightingMissIndex,
        ray, payload
    );

    return payload;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 getRandomPointOnAreaLight(uint randSeed, RealtimeAreaLight light)
{
    float A0 = light.PlaneA0, A1 = light.PlaneA1;
    float B0 = light.PlaneB0, B1 = light.PlaneB1;
    float C0 = light.PlaneC0, C1 = light.PlaneC1;

    float a  = A0 + nextRand(randSeed) * (A1 - A0);
    float b  = B0 + nextRand(randSeed) * (B1 - B0);
    float c  = C0 + nextRand(randSeed) * (C1 - C0);

    return float3(a, b, c);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 computeLighting(uint randSeed, float minT, float3 worldPos, float3 worldNorm)
{
    // The lighting is super dark, add this fudge factor for now
    // TODO: investigate this
    const float lightMultiplierFudge = 1.0f;

    // What we're going to return
    float3           lightResult = float3(0, 0, 0);

    // Pick a random light to sample
    int               lightIndex = min(int(nextRand(randSeed) * gSceneCB.NumLights), gSceneCB.NumLights - 1);
    RealtimeAreaLight light      = gLightsCB[lightIndex];
    float3            onLight    = getRandomPointOnAreaLight(randSeed, light);
    float3            lightDir   = normalize(onLight - worldPos);

    // Shoot a ray at the light
    DirectRayPayload lightPayload = areaLightVisibility(worldPos, lightDir, minT, SHADER_FLOAT_MAX);
    if (lightPayload.LightIndex >= 0)
    {
        float area            = light.AreaCoverage;
        float len             = lightPayload.HitBaryAndDist.a;
        float distanceSquared = len * len;
        float cosine          = saturate(dot(lightDir, worldNorm));
        float pdf             = distanceSquared / (cosine * area);

        if (cosine > 0 && dot(lightDir, worldNorm) >= 0)
        {
            lightResult = lightPayload.LightColor / pdf;
        }
    }

    return (lightResult * lightMultiplierFudge);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleDirectLighting(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    float3 shadeColor = float3(0, 0, 0);
    if (gSceneCB.NumLights > 0)
    {
        for (int i = 0; i < numRays; i++)
        {
            shadeColor += computeLighting(randSeed, minT, worldPos, worldNorm);
        }
    }
    else
    {
        shadeColor = float3(1, 1, 1);
    }

    // Modulate based on the physically based Lambertian term (albedo/pi)
    shadeColor *= (albedo.rgb / SHADER_PI);
    shadeColor /= numRays;

    return shadeColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 shootIndirectLightingRay(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm)
{
    float3 bounceDir = getCosHemisphereSample(randSeed, worldNorm);

    IndirectRayPayload payload;
    payload.Color   = float3(0, 0, 0);
    payload.RndSeed = randSeed;
    payload.NumRays = numRays;

    RayDesc ray;
    ray.Origin      = worldPos;
    ray.Direction   = bounceDir;
    ray.TMin        = minT;
    ray.TMax        = SHADER_FLOAT_MAX;

    TraceRay
    (
        gScene,
        RAY_FLAG_NONE,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.IndirectLightingHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.IndirectLightingMissIndex,
        ray, payload
    );

    return payload.Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleIndirectLighting(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm)
{
    // Get the bounce color from indirect lighting
    float3 bounceColor = float3(1, 1, 1);
    if (gSceneCB.NumLights > 0)
    {
        bounceColor = shootIndirectLightingRay(numRays, randSeed, minT, worldPos, worldNorm);
    }

    // The following uses this formula
    //   NdotL      = saturate(dot(worldNorm, bounceDir));
    //   sampleProb = (NdotL / SHADER_PI);
    //   shadeColor = (NdotL * bounceColor * albedo.rgb / SHADER_PI) / sampleProb;
    //
    // The terms cancel to this:
    float3 shadeColor = bounceColor;

    return shadeColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float shootAmbientOcclusionRays(int numRays, uint randSeed, float minT, float maxT, float3 worldPos, float3 worldNorm)
{
    // Sample cosine-weighted hemisphere around surface normal to pick a random ray direction
    float3 worldDir = getCosHemisphereSample(randSeed, worldNorm);

    // Accumulate AO
    float ao = 0.0f;
    for (int i = 0; i < numRays; i++)
    {
        ShadowRayPayload payload;
        payload.Value = 0.0f;

        RayDesc ray;
        ray.Origin    = worldPos;
        ray.Direction = worldDir;
        ray.TMin      = minT;
        ray.TMax      = maxT;

        // Trace our ray.  Ray stops after it's first definite hit; never execute closest hit shader
        TraceRay
        (
            gScene,
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
            RAYTRACING_INSTANCEMASK_OPAQUE,
            gSceneCB.AOHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.AOMissIndex,
            ray, payload
        );

        ao += payload.Value;
    }
    ao = ao / float(numRays);

    return ao;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void DirectLightingMiss(inout DirectRayPayload payload)
{
    // Missed the light
    payload.LightIndex = -1;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void DirectLightingClosest(inout DirectRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    // Hit the light
    payload.LightIndex     = gInstanceDataCB.LightIndex;
    payload.HitBaryAndDist = float4(barycentrics, RayTCurrent());
    payload.LightColor     = gMaterial.Diffuse.rgb;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void IndirectLightingMiss(inout IndirectRayPayload payload)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void IndirectLightingClosest(inout IndirectRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3               bary       = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    RealtimeSceneVertex  vert       = getVertex(PrimitiveIndex(), bary);
    float3               worldPos   = mul(vert.Position, (float3x3)gInstanceDataCB.WorldMatrix);
    float3               worldNorm  = mul(vert.Normal, (float3x3)gInstanceDataCB.WorldMatrix);
    int                  numRays    = payload.NumRays;

    payload.Color = float3(0, 0, 0);
    for (int i = 0; i < numRays; i++)
    {
        payload.Color += computeLighting(payload.RndSeed, RayTMin(), worldPos, worldNorm);
    }

    // Read albedo
    float3 albedo = gAlbedoArray[gMaterial.DiffuseTextureId][vert.TexCoord].xyz;

    // Modulate based on the physically based Lambertian term (albedo/pi)
    payload.Color *= (albedo / SHADER_PI);
    payload.Color /= numRays;
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

inline float4 temporalAccumulate(float4 prevColor, float4 currColor)
{
    return (gSceneCB.AccumCount * prevColor + currColor) / (gSceneCB.AccumCount + 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("raygeneration")]
void RayGeneration()
{
    // Inputs
    int     numRays      = gSceneCB.NumRays;
    uint3   launchIndex  = DispatchRaysIndex();
    uint3   launchDim    = DispatchRaysDimensions();
    uint    randSeed     = initRand(launchIndex.x + launchIndex.y * launchDim.x, gSceneCB.FrameCount, 16);
    float4  worldPos     = gPositions[launchIndex.xy];
    float3  worldNorm    = gNormals[launchIndex.xy].xyz;
    float2  uvCoord      = gTexCoordsAndDepth[launchIndex.xy].xy;
    float4  albedo       = gAlbedo[launchIndex.xy];
    float   aoRadius     = gSceneCB.AORadius;
    float   minT         = 0.01f;

    // Get direct, indirect and AO contributions
    float3 direct        = sampleDirectLighting(numRays, randSeed, minT, worldPos.xyz, worldNorm, albedo);
    float3 indirect      = sampleIndirectLighting(numRays, randSeed, minT, worldPos.xyz, worldNorm);
    float  ao            = shootAmbientOcclusionRays(numRays, randSeed, minT, aoRadius, worldPos.xyz, worldNorm);
    
    // Pass through background/lights
    float2 backSelect    = float2(worldPos.w, 1 - worldPos.w);
    direct               = (direct * backSelect.x)   + (albedo.rgb * backSelect.y);
    indirect             = (indirect * backSelect.x) + (float3(0, 0, 0) * backSelect.y);
    ao                   = (ao * backSelect.x)       + (1.0f * backSelect.y);

    // Accumulate results
    float4 finalDirectAO = temporalAccumulate(gPrevDirectAO[launchIndex.xy], float4(direct, ao));
    float4 finalIndirect = temporalAccumulate(gPrevIndirect[launchIndex.xy], float4(indirect, 1));

    // Zero out nan, inf, etc.
    finalDirectAO = any(isnan(finalDirectAO)) ? float4(1, 1, 1, 1) : finalDirectAO;
    finalIndirect = any(isnan(finalIndirect)) ? float4(1, 1, 1, 1) : finalIndirect;

    // Write final results to the output
    gCurrDirectAO[launchIndex.xy] = finalDirectAO;
    gCurrIndirect[launchIndex.xy] = finalIndirect;
}
