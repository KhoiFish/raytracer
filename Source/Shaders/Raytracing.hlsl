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

RWTexture2D<float4>                         gPrevOutput         : register(u0);
RWTexture2D<float4>                         gCurOutput          : register(u1);

ConstantBuffer<RaytracingGlobalCB>          gSceneCB            : register(b0);
ConstantBuffer<RenderNodeInstanceData>      gInstanceCB         : register(b1);
ConstantBuffer<RenderMaterial>              gMaterial           : register(b2);

RaytracingAccelerationStructure             gScene              : register(t0);
Texture2D<float4>                           gPositions          : register(t1);
Texture2D<float4>                           gNormals            : register(t2);
Texture2D<float4>                           gTexCoordsAndDepth  : register(t3);
Texture2D<float4>                           gAlbedo             : register(t4);
ByteAddressBuffer                           gVertexBuffer       : register(t5);
ByteAddressBuffer                           gIndexBuffer        : register(t6);

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

inline float shadowRayVisibility(float3 origin, float3 direction, float minT, float maxT)
{
    RayDesc ray;
    ray.Origin      = origin;
    ray.Direction   = direction;
    ray.TMin        = minT;
    ray.TMax        = maxT;

    RayPayload payload = { 0.0f, float4(0, 0, 0, 0) };

    TraceRay
    (
        gScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.AOHitGroupIndex, gSceneCB.HitProgramCount, gSceneCB.AOMissIndex,
        ray, payload);

    return payload.Value;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline RayPayload areaLightVisibility(float3 orig, float3 dir, float minT, float maxT)
{
    RayPayload payload = { 0.0f, float4(0, 0, 0, 0) };

    RayDesc ray;
    ray.Origin    = orig;
    ray.Direction = dir;
    ray.TMin      = minT;
    ray.TMax      = maxT;

    TraceRay
    (
        gScene,
        RAY_FLAG_NONE,
        RAYTRACING_INSTANCEMASK_AREALIGHT,
        gSceneCB.DirectLightingHitGroupIndex, gSceneCB.HitProgramCount, gSceneCB.DirectLightingMissIndex,
        ray, payload
    );

    return payload;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float computeLighting(uint randSeed, float minT, float3 worldPos, float3 worldNorm)
{
    // Shoot rays brute force to see if we hit an area light
    // TODO: bias the sample towards lights and weight with a pdf
    float      lightResult     = 0;
    float3     worldDir        = getCosHemisphereSample(randSeed, worldNorm);
    RayPayload hitLightPayload = areaLightVisibility(worldPos, worldDir, minT, SHADER_FLOAT_MAX);
    if (hitLightPayload.Value > 0)
    {
        float distToLight = hitLightPayload.HitBaryAndDist.a;
        float LdotN       = saturate(dot(worldNorm, worldDir));
        float shadowMult  = shadowRayVisibility(worldPos, worldDir, minT, SHADER_FLOAT_MAX);

        lightResult = LdotN * shadowMult;
    }

    return lightResult;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleDirectLighting(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    float3 shadeColor     = float3(0, 0, 0);
    float3 lightIntensity = float3(100.0f, 100.0f, 100.0f);
    for(int i = 0; i < numRays; i++)
    {
        shadeColor += computeLighting(randSeed, minT, worldPos, worldNorm) * lightIntensity;
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

    // Setup shadow ray
    RayDesc ray;
    ray.Origin      = worldPos;
    ray.Direction   = bounceDir;
    ray.TMin        = minT;
    ray.TMax        = SHADER_FLOAT_MAX;

    // Initialize the ray's payload data with black return color and the current rng seed
    IndirectRayPayload payload;
    payload.Color   = float3(0, 0, 0);
    payload.RndSeed = randSeed;
    payload.NumRays = numRays;

    // Trace ray
    TraceRay
    (
        gScene,
        RAY_FLAG_NONE,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.IndirectLightingHitGroupIndex, gSceneCB.HitProgramCount, gSceneCB.IndirectLightingMissIndex,
        ray, payload
    );

    return payload.Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 sampleIndirectLighting(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    // Get the bounce color from indirect lighting
    float3 bounceColor = shootIndirectLightingRay(numRays, randSeed, minT, worldPos, worldNorm);

    // The following uses this formula
    //   NdotL      = saturate(dot(worldNorm, bounceDir));
    //   sampleProb = (NdotL / SHADER_PI);
    //   shadeColor = (NdotL * bounceColor * albedo.rgb / SHADER_PI) / sampleProb;
    //
    // The terms cancel to this:
    float3 shadeColor = bounceColor; //albedo.rgb * bounceColor;

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
        RayPayload payload = { 0.0f, float4(0, 0, 0, 0) };

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
            gSceneCB.AOHitGroupIndex, gSceneCB.HitProgramCount, gSceneCB.AOMissIndex,
            ray, payload
        );

        ao += payload.Value;
    }
    ao = ao / float(numRays);

    return ao;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void DirectLightingMiss(inout RayPayload payload)
{
    // Missed the light
    payload.Value = 0.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void DirectLightingClosest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    // Hit the light
    payload.Value          = 1.0f;
    payload.HitBaryAndDist = float4(barycentrics, RayTCurrent());
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
    float3               worldPos   = mul(vert.Position, (float3x3)gInstanceCB.WorldMatrix);
    float3               worldNorm  = mul(vert.Normal, (float3x3)gInstanceCB.WorldMatrix);

#if 1
    int numRays = payload.NumRays;
    float3 shadeColor     = float3(0, 0, 0);
    for (int i = 0; i < numRays; i++)
    {
        shadeColor += computeLighting(payload.RndSeed, RayTMin(), worldPos, worldNorm);
    }

    // Modulate based on the physically based Lambertian term (albedo/pi)
    shadeColor *= (gMaterial.Diffuse.rgb / SHADER_PI);
    shadeColor /= numRays;
    

    payload.Color = shadeColor;
#else
    float shadowMult = 0.5f; // shadowRayVisibility(worldPos, worldNorm, RayTMin(), SHADER_FLOAT_MAX);

    payload.Color = gMaterial.Diffuse.rgb * shadowMult;
#endif
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void AOMiss(inout RayPayload payload)
{
    // Our ambient occlusion value is 1 if we hit nothing
    payload.Value = 1.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void AOClosest(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("raygeneration")]
void RayGeneration()
{
    int     numRays     = gSceneCB.NumRays;
    uint3   launchIndex = DispatchRaysIndex();
    uint3   launchDim   = DispatchRaysDimensions();
    uint    randSeed    = initRand(launchIndex.x + launchIndex.y * launchDim.x, gSceneCB.FrameCount, 16);
    float3  worldPos    = gPositions[launchIndex.xy].xyz;
    float3  worldNorm   = gNormals[launchIndex.xy].xyz;
    float4  albedo      = gAlbedo[launchIndex.xy];
    float   aoRadius    = gSceneCB.AORadius;
    float   minT        = 0.01f;

    float3 direct       = sampleDirectLighting(numRays, randSeed, minT, worldPos, worldNorm, albedo);
    float3 indirect     = sampleIndirectLighting(numRays, randSeed, minT, worldPos, worldNorm, albedo) * 10;
    float  ao           = shootAmbientOcclusionRays(numRays, randSeed, minT, aoRadius, worldPos.xyz, worldNorm);

    float4 curColor     = float4(indirect + direct, ao);
    //float4 curColor     = float4(direct, ao);
    float4 prevColor    = gPrevOutput[launchIndex.xy];
    float4 finalColor   = (gSceneCB.AccumCount * prevColor + curColor) / (gSceneCB.AccumCount + 1);

    // Write result to the output
    gCurOutput[launchIndex.xy] = finalColor;
}
