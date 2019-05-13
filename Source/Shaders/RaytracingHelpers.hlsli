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

#define INFINITY (1.0/0.0)

// ----------------------------------------------------------------------------------------------------------------------------

struct Ray
{
    float3 origin;
    float3 direction;
};

// ----------------------------------------------------------------------------------------------------------------------------

float LengthToPow2(float2 p)
{
    return dot(p, p);
}

// ----------------------------------------------------------------------------------------------------------------------------

float LengthToPow2(float3 p)
{
    return dot(p, p);
}

// ----------------------------------------------------------------------------------------------------------------------------

float CalculateAnimationInterpolant(in float elapsedTime, in float cycleDuration)
{
    float curLinearCycleTime = fmod(elapsedTime, cycleDuration) / cycleDuration;
    curLinearCycleTime = (curLinearCycleTime <= 0.5f) ? 2 * curLinearCycleTime : 1 - 2 * (curLinearCycleTime - 0.5f);
    return smoothstep(0, 1, curLinearCycleTime);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Swap(inout float a, inout float b)
{
    float temp = a;
    a = b;
    b = temp;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool IsInRange(in float val, in float min, in float max)
{
    return (val >= min && val <= max);
}

// ----------------------------------------------------------------------------------------------------------------------------

static uint3 Load3x16BitIndices(uint offsetBytes, ByteAddressBuffer Indices)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

// ----------------------------------------------------------------------------------------------------------------------------

float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// ----------------------------------------------------------------------------------------------------------------------------

float3 HitAttribute(float3 vertexAttribute[3], float2 barycentrics)
{
    return vertexAttribute[0] +
        barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
    world.xyz /= world.w;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = normalize(world.xyz - ray.origin);

    return ray;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool IsCulled(in Ray ray, in float3 hitSurfaceNormal)
{
    float rayDirectionNormalDot = dot(ray.direction, hitSurfaceNormal);

    bool isCulled =
        ((RayFlags() & RAY_FLAG_CULL_BACK_FACING_TRIANGLES) && (rayDirectionNormalDot > 0))
        ||
        ((RayFlags() & RAY_FLAG_CULL_FRONT_FACING_TRIANGLES) && (rayDirectionNormalDot < 0));

    return isCulled;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool IsAValidHit(in Ray ray, in float thit, in float3 hitSurfaceNormal)
{
    return IsInRange(thit, RayTMin(), RayTCurrent()) && !IsCulled(ray, hitSurfaceNormal);
}

// ----------------------------------------------------------------------------------------------------------------------------

float2 TexCoords(in float3 position)
{
    return position.xz;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CalculateRayDifferentials(out float2 ddx_uv, out float2 ddy_uv, in float2 uv, in float3 hitPosition, in float3 surfaceNormal, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    // Compute ray differentials by intersecting the tangent plane to the  surface.
    Ray ddx = GenerateCameraRay(DispatchRaysIndex().xy + uint2(1, 0), cameraPosition, projectionToWorld);
    Ray ddy = GenerateCameraRay(DispatchRaysIndex().xy + uint2(0, 1), cameraPosition, projectionToWorld);

    // Compute ray differentials.
    float3 ddx_pos = ddx.origin - ddx.direction * dot(ddx.origin - hitPosition, surfaceNormal) / dot(ddx.direction, surfaceNormal);
    float3 ddy_pos = ddy.origin - ddy.direction * dot(ddy.origin - hitPosition, surfaceNormal) / dot(ddy.direction, surfaceNormal);

    // Calculate texture sampling footprint.
    ddx_uv = TexCoords(ddx_pos) - uv;
    ddy_uv = TexCoords(ddy_pos) - uv;
}

// ----------------------------------------------------------------------------------------------------------------------------

float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 f0)
{
    float cosi = saturate(dot(-I, N));
    return f0 + (1 - f0) * pow(1 - cosi, 5);
}
