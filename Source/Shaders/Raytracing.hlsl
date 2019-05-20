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

RWTexture2D<float4>                 gPrevOutput         : register(u0);
RWTexture2D<float4>                 gCurOutput          : register(u1);
RaytracingAccelerationStructure     gScene              : register(t0);
Texture2D<float4>                   gPositions          : register(t1);
Texture2D<float4>                   gNormals            : register(t2);
Texture2D<float4>                   gTexCoordsAndDepth  : register(t3);
Texture2D<float4>                   gAlbedo             : register(t4);
ConstantBuffer<RaytracingGlobalCB>  gSceneCB            : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

inline float shadowRayVisibility(float3 origin, float3 direction, float minT, float maxT)
{
    // Setup our shadow ray
    RayDesc ray;
    ray.Origin      = origin;
    ray.Direction   = direction;
    ray.TMin        = minT;
    ray.TMax        = maxT;

    // Our shadow rays are *assumed* to hit geometry; this miss shader changes this to 1.0 for "visible"
    RayPayload payload = { 0.0f, float4(0, 0, 0, 0) };

    // Query if anything is between the current point and the light
    TraceRay
    (
        gScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        RAYTRACING_INSTANCEMASK_OPAQUE,
        gSceneCB.AOHitGroupIndex, gSceneCB.HitProgramCount, gSceneCB.AOMissIndex,
        ray, payload);

    // Return our ray payload (which is 1 for visible, 0 for occluded)
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

inline float3 shootLambertShadowsRays(int numRays, uint randSeed, float minT, float3 worldPos, float3 worldNorm, float4 albedo)
{
    float3 shadeColor = albedo.rgb;

    //if (worldPos.w != 0.0f)
    {
        // Shoot rays brute force to see if we hit an area light
        // TODO: bias the sample towards lights and weight with a pdf
        shadeColor = float3(0.0, 0.0, 0.0);
        for(int i = 0; i < numRays; i++)
        {
            float3 lightIntensity = float3(5, 5, 5);

            // See if we hit an area light
            float3     worldDir        = getCosHemisphereSample(randSeed, worldNorm);
            RayPayload hitLightPayload = areaLightVisibility(worldPos, worldDir, minT, 1000000);
            if (hitLightPayload.Value > 0)
            {
                float distToLight = hitLightPayload.HitBaryAndDist.a;
                float LdotN       = saturate(dot(worldNorm, worldDir));
                float shadowMult  = shadowRayVisibility(worldPos, worldDir, minT, distToLight);

                shadeColor += LdotN * shadowMult * lightIntensity;
            }
        }

        // Modulate based on the physically based Lambertian term (albedo/pi)
        shadeColor *= albedo.rgb / 3.141592f;
    }

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

[shader("raygeneration")]
void RayGenerationShader()
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

    // Get lambert and ao terms
    float3 lambert = shootLambertShadowsRays(numRays, randSeed, minT, worldPos, worldNorm, albedo);
    float  ao      = shootAmbientOcclusionRays(numRays, randSeed, minT, aoRadius, worldPos.xyz, worldNorm);

    // Save out results
    float4 curColor   = float4(lambert, ao);
    float4 prevColor  = gPrevOutput[launchIndex.xy];
    float4 finalColor = (gSceneCB.AccumCount * prevColor + curColor) / (gSceneCB.AccumCount + 1);

    gCurOutput[launchIndex.xy] = finalColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void DirectLightingMissShader(inout RayPayload payload)
{
    // Missed the light
    payload.Value = 0.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void DirectLightingClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1.f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    // Hit the light
    payload.Value          = 1.0f;
    payload.HitBaryAndDist = float4(barycentrics, RayTCurrent());
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void AOMissShader(inout RayPayload payload)
{
    // Our ambient occlusion value is 1 if we hit nothing.
    payload.Value = 1.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void AOClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    ;
}
