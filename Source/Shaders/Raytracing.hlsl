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
#include "../RealtimeEngine/ShaderStructs.h"
#include "RaytracingHelpers.hlsli"

// ----------------------------------------------------------------------------------------------------------------------------

RWTexture2D<float4>                 gRenderTarget : register(u0);
//ConstantBuffer<SceneConstantBuffer> gSceneCB      : register(b0);
RaytracingAccelerationStructure     gScene        : register(t1, space0);
ByteAddressBuffer                   gIndices      : register(t2, space0);
StructuredBuffer<SceneVertex>       gVertices     : register(t3, space0);

// ----------------------------------------------------------------------------------------------------------------------------

float4 TraceRadianceRay(in Ray ray, in UINT currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }

    // Set the ray's extents.
    // Set TMin to a zero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable face culling so as to avoid surface face fighting.
    RayDesc rayDesc;
    rayDesc.Origin    = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin      = 0;
    rayDesc.TMax      = 10000;

    RayPayload rayPayload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1 };

    // Trae the ray
    TraceRay
    (
        gScene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        TraceRayParameters::InstanceMask,
        TraceRayParameters::HitGroup::Offset[RayType::Radiance],
        TraceRayParameters::HitGroup::GeometryStride,
        TraceRayParameters::MissShader::Offset[RayType::Radiance],
        rayDesc, rayPayload
    );

    return rayPayload.Color;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("raygeneration")]
void RayGenerationShader()
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    //Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, gSceneCB.CameraPosition.xyz, gSceneCB.InverseViewProjectionMatrix);

    float4x4 mat;
    Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, float3(0, 0, 0), mat);

    // Cast a ray into the scene and retrieve a shaded color.
    UINT   currentRecursionDepth = 0;
    float4 color                 = TraceRadianceRay(ray, currentRecursionDepth);

    // Write the raytraced color to the output texture.
    gRenderTarget[DispatchRaysIndex().xy] = color;
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("closesthit")]
void ClosestHitShader(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up three 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex, gIndices);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 triangleNormal = gVertices[indices[0]].Normal;

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
#if 0
    // Shadow component.
    // Trace a shadow ray.
    //float3 hitPosition = HitWorldPosition();
    //Ray shadowRay = { hitPosition, normalize(gSceneCB.LightPosition.xyz - hitPosition) };
    //bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);
    bool shadowRayHit = false;

    //float checkers = AnalyticalCheckersTexture(HitWorldPosition(), triangleNormal, gSceneCB.CameraPosition.xyz, gSceneCB.InverseViewProjectionMatrix);
    float checkers = 1.f;

    // Reflected component.
    float4 reflectedColor = float4(0, 0, 0, 0);
    if (l_materialCB.reflectanceCoef > 0.001)
    {
        // Trace a reflection ray.
        Ray reflectionRay = { HitWorldPosition(), reflect(WorldRayDirection(), triangleNormal) };
        float4 reflectionColor = TraceRadianceRay(reflectionRay, rayPayload.recursionDepth);

        float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), triangleNormal, l_materialCB.albedo.xyz);
        reflectedColor = l_materialCB.reflectanceCoef * float4(fresnelR, 1) * reflectionColor;
    }

    // Calculate final color.
    float4 phongColor = CalculatePhongLighting(l_materialCB.albedo, triangleNormal, shadowRayHit, l_materialCB.diffuseCoef, l_materialCB.specularCoef, l_materialCB.specularPower);
    float4 color = checkers * (phongColor + reflectedColor);

    // Apply visibility falloff.
    float t = RayTCurrent();
    color = lerp(color, BackgroundColor, 1.0 - exp(-0.000002 * t * t * t));

    rayPayload.color = color;
#endif

    rayPayload.Color = float4(1, 0, 0, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

[shader("miss")]
void MissShader(inout RayPayload rayPayload)
{
    float4 backgroundColor = float4(BackgroundColor);
    rayPayload.Color = backgroundColor;
}
