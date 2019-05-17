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
#include "../RealtimeEngine/RenderSceneVertex.h"
#include "../RealtimeEngine/RenderSceneShader.h"
#include "../RaytracingShader.h"
#include "RaytracingHelpers.hlsli"

// ----------------------------------------------------------------------------------------------------------------------------

RWTexture2D<float4>                 gOutput  : register(u0);
RaytracingAccelerationStructure     gScene   : register(t0);
ConstantBuffer<SceneConstantBuffer> gSceneCB : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 linearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1  = sqrt(c);
    float3 sq2  = sqrt(sq1);
    float3 sq3  = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;

    return srgb;
}

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

[shader("raygeneration")]
void RayGenerationShader()
{
    uint3 launchIndex = DispatchRaysIndex();

    RayDesc ray;
    ray.TMin = 0;
    ray.TMax = 100000;
    generateCameraRay(launchIndex.xy, ray.Origin, ray.Direction);

    RayPayload payload;
    TraceRay(gScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload);

    float3 col = linearToSrgb(payload.Color);
    gOutput[launchIndex.xy] = float4(col, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.Color = float3(0.4, 0.6, 0.2);
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.Color = float3(1, 0, 0);
}

