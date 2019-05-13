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

RaytracingAccelerationStructure     gScene        : register(t0, space0);
RWTexture2D<float4>                 gRenderTarget : register(u0);
ConstantBuffer<SceneConstantBuffer> gSceneCB      : register(b0);
ByteAddressBuffer                   gIndices      : register(t1, space0);
StructuredBuffer<SceneVertex>       gVertices     : register(t2, space0);

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
    Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, gSceneCB.CameraPosition.xyz, gSceneCB.InverseViewProjectionMatrix);

    // Cast a ray into the scene and retrieve a shaded color.
    UINT   currentRecursionDepth = 0;
    float4 color                 = TraceRadianceRay(ray, currentRecursionDepth);

    // Write the raytraced color to the output texture.
    gRenderTarget[DispatchRaysIndex().xy] = color;
}
