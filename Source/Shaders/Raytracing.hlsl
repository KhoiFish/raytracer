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

RWTexture2D<float4>                 gPrevAO             : register(u0);
RWTexture2D<float4>                 gCurAO              : register(u1);
RaytracingAccelerationStructure     gScene              : register(t0);
Texture2D<float4>                   gPositions          : register(t1);
Texture2D<float4>                   gNormals            : register(t2);
Texture2D<float4>                   gTexCoordsAndDepth  : register(t3);
Texture2D<float4>                   gAlbedo             : register(t4);
ConstantBuffer<RaytracingGlobalCB>  gSceneCB            : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

inline void generateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    // Center in the middle of the pixel
    float2 xy        = index + 0.5;
    float2 screenPos = xy / gSceneCB.OutputResolution * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates
    screenPos.y = -screenPos.y;

    // Unproject into a ray
    float4 unprojected = mul(float4(screenPos, 0, 1), gSceneCB.InverseTransposeViewProjectionMatrix);
    float3 world       = unprojected.xyz / unprojected.w;

    // We got our ray
    origin    = gSceneCB.CameraPosition.xyz;
    direction = normalize(world - origin);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 getWorldPositionFromDepth(uint3 dispatchRaysIndex)
{
    float2 xy           = dispatchRaysIndex.xy + 0.5;
    float2 screenPos    = (xy / gSceneCB.OutputResolution * 2.0 - 1.0) * float2(1, -1);
    float  sceneDepth   = gTexCoordsAndDepth.Load(int3(xy, 0)).z;
    float4 unprojected  = mul(float4(screenPos, sceneDepth, 1), gSceneCB.InverseTransposeViewProjectionMatrix);
    float3 world        = unprojected.xyz / unprojected.w;

    return world;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float shootAmbientOcclusionRay(float3 orig, float3 dir, float minT, float maxT)
{
    RayPayload rayPayload = { 0.0f };

    RayDesc rayAO;
    rayAO.Origin    = orig;
    rayAO.Direction = dir;
    rayAO.TMin      = minT;
    rayAO.TMax      = maxT;

    // Trace our ray.  Ray stops after it's first definite hit; never execute closest hit shader
    TraceRay
    (
        gScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 
        gSceneCB.AOHitGroupIndex, 0, gSceneCB.AOMissIndex,
        rayAO, rayPayload
    );

    return rayPayload.AOValue;
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
    float   ao          = float(numRays);
    float   aoRadius    = gSceneCB.AORadius;
    float   minT        = 0.01f;
    
    // Accumulate AO
    ao = 0.0f;
    for (int i = 0; i < numRays; i++)
    {
        // Sample cosine-weighted hemisphere around surface normal to pick a random ray direction
        float3 worldDir = getCosHemisphereSample(randSeed, worldNorm);

        // Shoot our ambient occlusion ray and update the value we'll output with the result
        ao += shootAmbientOcclusionRay(worldPos.xyz, worldDir, minT, aoRadius);
    }

    // Save out our AO color
    float  aoColor    = ao / float(numRays);
    float4 curColor   = float4(aoColor, aoColor, aoColor, 1.0f);
    float4 prevColor  = gPrevAO[launchIndex.xy];
    float4 finalColor = (gSceneCB.AccumCount * prevColor + curColor) / (gSceneCB.AccumCount + 1);

    gCurAO[launchIndex.xy] = finalColor;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void AOMissShader(inout RayPayload payload)
{
    // Our ambient occlusion value is 1 if we hit nothing.
    payload.AOValue = 1.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void AOClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void DirectLightingMissShader(inout RayPayload payload)
{
    
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void DirectLightingClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    
}