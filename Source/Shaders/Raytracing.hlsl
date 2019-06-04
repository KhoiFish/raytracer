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
RWTexture2D<float4>                         gPrevDirectAO         : register(u0, space0);
RWTexture2D<float4>                         gCurrDirectAO         : register(u1, space0);
RWTexture2D<float4>                         gPrevIndirect         : register(u2, space0);
RWTexture2D<float4>                         gCurrIndirect         : register(u3, space0);

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

    const uint3 indices = getIndices(triangleIndex);
    const int   stride  = (3 * 4) + (3 * 4) + (2 * 4);    
    for (uint i = 0; i < 3; i++)
    {
        int address = indices[i] * stride;

        v.Position += asfloat(gVertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.Normal += asfloat(gVertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.TexCoord += asfloat(gVertexBuffer.Load2(address)) * barycentrics[i];
    }

    return v;
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
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
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

inline DirectRayPayload shootAreaLightVisibilityRay(float3 orig, float3 dir, float minT, float maxT)
{
    DirectRayPayload payload;
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
        gSceneCB.DirectLightingHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.DirectLightingMissIndex,
        ray, payload
    );

    return payload;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 shootIndirectLightingRay(uint randSeed, float minT, float3 worldPos, float3 worldNorm, uint curDepth)
{
    float3 bounceDir = getCosHemisphereSample(randSeed, worldNorm);

    IndirectRayPayload payload;
    payload.Color    = float3(0, 0, 0);
    payload.RndSeed  = randSeed;
    payload.RayDepth = curDepth + 1;

    RayDesc ray;
    ray.Origin      = worldPos;
    ray.Direction   = bounceDir;
    ray.TMin        = minT;
    ray.TMax        = SHADER_FLOAT_MAX;

    TraceRay
    (
        gScene,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.IndirectLightingHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.IndirectLightingMissIndex,
        ray, payload
    );

    return payload.Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 shootColorRay(uint randSeed, float minT, float3 origin, float3 dir, uint curDepth)
{
    ColorRayPayload payload;
    payload.Color    = float3(0, 0, 0);

    RayDesc ray;
    ray.Origin      = origin;
    ray.Direction   = dir;
    ray.TMin        = minT;
    ray.TMax        = SHADER_FLOAT_MAX;

    TraceRay
    (
        gScene,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        RAYTRACING_INSTANCEMASK_ALL,
        gSceneCB.ColorHitGroupIndex, RAYTRACING_NUM_HIT_PROGRAMS, gSceneCB.ColorMissIndex,
        ray, payload
    );

    return payload.Color;
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

inline float3 shadeLambert(RenderMaterial material, int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    float3 shadeResult = float3(0, 0, 0);
    for (int i = 0; i < numRays; i++)
    {
        // Pick a random light to sample
        float3            lightResult = float3(0, 0, 0);
        int               lightIndex  = min(int(nextRand(randSeed) * gSceneCB.NumLights), gSceneCB.NumLights - 1);
        RealtimeAreaLight light       = gLightsCB[lightIndex];
        float3            onLight     = getRandomPointOnAreaLight(randSeed, light);
        float3            lightDir    = normalize(onLight - worldPos);
        float             dist        = length(onLight - worldPos);

        // Shoot a ray at the light
        DirectRayPayload lightPayload = shootAreaLightVisibilityRay(worldPos, lightDir, minT, SHADER_FLOAT_MAX);
        if (lightPayload.LightIndex >= 0)
        {
            float area            = light.AreaCoverage;
            float len             = dist;
            float distanceSquared = len * len;
            float cosine          = saturate(dot(lightDir, worldNorm));
            float pdf             = distanceSquared / (cosine * area);

            if (cosine > 0 && dot(lightDir, worldNorm) >= 0)
            {
                shadeResult += lightPayload.LightColor / pdf;
            }
        }
    }

    // Modulate based on the physically based Lambertian term (albedo/pi)
    shadeResult *= (albedo.rgb / SHADER_PI);
    shadeResult /= numRays;

    return shadeResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 shadeMetal(RenderMaterial material, float3 incomingRayDir, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo, uint curDepth)
{
    float3 reflDir = reflect(incomingRayDir, worldNorm) + (material.Fuzz * getCosHemisphereSample(randSeed, worldNorm));

    if (dot(reflDir, worldNorm) > 0.0f)
    {
        return albedo.rgb * shootColorRay(randSeed, minT, worldPos, reflDir, curDepth);
    }

    return float3(0, 0, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 shade(RenderMaterial material, float3 incomingRayDir, int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo, uint curDepth)
{
    float3 shadeColor;
    switch (material.Type)
    {
        case RenderMaterialType_Light:
            shadeColor = material.Diffuse.rgb;
        break;

        case RenderMaterialType_Lambert:
            shadeColor = shadeLambert(material, numRays, randSeed, minT, worldPos, worldNorm, albedo);
        break;

        case RenderMaterialType_Metal:
            shadeColor = shadeMetal(material, incomingRayDir, randSeed, minT, worldPos, worldNorm, albedo, curDepth);
        break;
    }

    return shadeColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleDirectLighting(RenderMaterial material, float3 incomingRayDir, int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    float3 shadeColor;
    if (gSceneCB.NumLights > 0)
    {
        shadeColor = shade(material, incomingRayDir, numRays, randSeed, minT, worldPos, worldNorm, albedo, 0);
    }
    else
    {
        shadeColor = albedo.rgb;
        shadeColor /= numRays;
    }

    return shadeColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleIndirectLighting(uint randSeed, float minT, float3 worldPos, float3 worldNorm)
{
    // Get the bounce color from indirect lighting
    float3 bounceColor = float3(1, 1, 1);
    if (gSceneCB.NumLights > 0)
    {
        bounceColor = shootIndirectLightingRay(randSeed, minT, worldPos, worldNorm, 0);
    }

    return bounceColor;
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
    payload.LightIndex = gInstanceDataCB.LightIndex;
    payload.LightColor = gMaterial.Diffuse.rgb;
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
    if (payload.RayDepth < gSceneCB.MaxRayDepth)
    {
        // Hit an opaque
        float3               bary       = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
        RealtimeSceneVertex  vert       = getVertex(PrimitiveIndex(), bary);
        float3               worldPos   = mul(vert.Position, (float3x3)gInstanceDataCB.WorldMatrix);
        float3               worldNorm  = mul(vert.Normal, (float3x3)gInstanceDataCB.WorldMatrix);
        float4               albedo     = gAlbedoArray[gMaterial.DiffuseTextureId].SampleLevel(AnisoRepeatSampler, vert.TexCoord, 0);

        payload.Color = shade(gMaterial, WorldRayDirection(), gSceneCB.NumRays, payload.RndSeed, RayTMin(), worldPos, worldNorm, albedo, payload.RayDepth);
    }
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
void ColorMiss(inout ColorRayPayload payload)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void ColorClosest(inout ColorRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3               bary       = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    RealtimeSceneVertex  vert       = getVertex(PrimitiveIndex(), bary);
    float3               worldPos   = mul(vert.Position, (float3x3)gInstanceDataCB.WorldMatrix);
    float3               worldNorm  = mul(vert.Normal, (float3x3)gInstanceDataCB.WorldMatrix);
    float4               albedo     = gAlbedoArray[gMaterial.DiffuseTextureId].SampleLevel(AnisoRepeatSampler, vert.TexCoord, 0);

    // It's cheaper to just return the albedo color, instead of doing a full shade
    payload.Color = albedo.rgb;
    //payload.Color = shade(gMaterial, WorldRayDirection(), gSceneCB.NumRays, payload.RndSeed, RayTMin(), worldPos, worldNorm, albedo, payload.RayDepth);
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
    int             numRays     = gSceneCB.NumRays;
    uint3           launchIndex = DispatchRaysIndex();
    uint3           launchDim   = DispatchRaysDimensions();
    uint            randSeed    = initRand(launchIndex.x + launchIndex.y * launchDim.x, gSceneCB.FrameCount, 16);
    float4          worldPos    = gPositions[launchIndex.xy];
    float3          rayInDir    = normalize(worldPos.xyz - gSceneCB.CameraPosition.xyz);
    float4          normAndId   = gNormals[launchIndex.xy];
    float3          worldNorm   = normalize(normAndId.xyz);
    int             instanceId  = int(normAndId.w);
    RenderMaterial  material    = gMaterialsCB[instanceId];
    float4          albedo      = gAlbedo[launchIndex.xy];
    float           aoRadius    = gSceneCB.AORadius;
    float           minT        = 0.001f;

    // Get direct, indirect and AO contributions
    float3 direct   = sampleDirectLighting(material, rayInDir, numRays, randSeed, minT, worldPos.xyz, worldNorm, albedo);
    float3 indirect = sampleIndirectLighting(randSeed, minT, worldPos.xyz, worldNorm);
    float  ao       = shootAmbientOcclusionRays(numRays, randSeed, minT, aoRadius, worldPos.xyz, worldNorm);
    
    // Pass through background/lights
    float2 backSelect = float2(worldPos.w, 1 - worldPos.w);
    direct            = (direct * backSelect.x)   + (albedo.rgb * backSelect.y);
    indirect          = (indirect * backSelect.x) + (float3(0, 0, 0) * backSelect.y);
    ao                = (ao * backSelect.x)       + (1.0f * backSelect.y);

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
