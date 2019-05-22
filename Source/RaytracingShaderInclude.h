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

#ifndef RAYTRACINGSHADER_H
#define RAYTRACINGSHADER_H

#include "RealtimeEngine/ShaderCompat.h"

// ----------------------------------------------------------------------------------------------------------------------------

static const XMFLOAT4   ChromiumReflectance = XMFLOAT4(0.549f, 0.556f, 0.554f, 1.0f);
static const XMFLOAT4   BackgroundColor     = XMFLOAT4(0.8f, 0.9f, 1.0f, 1.0f);
static const float      InShadowRadiance    = 0.35f;

// ----------------------------------------------------------------------------------------------------------------------------

struct ShadowRayPayload
{
    float       Value;
};

struct DirectRayPayload
{
    float       Value;
    XMFLOAT4    HitBaryAndDist;
    XMFLOAT3    LightColor;
};

struct IndirectRayPayload
{
    XMFLOAT3    Color;
    UINT        RndSeed;
    UINT        NumRays;
};

// Set this to the largest payload struct from above
#define RAYTRACER_MAX_PAYLOAD_SIZE  sizeof(DirectRayPayload)

// ----------------------------------------------------------------------------------------------------------------------------

namespace RayType
{
    enum Enum
    {
        Radiance = 0,   // ~ Primary, reflected camera/view rays calculating color for each hit.
        Shadow,         // ~ Shadow/visibility rays, only testing for occlusion
        Count
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

namespace TraceRayParameters
{
    static const UINT InstanceMask = ~0;   // Everything is visible.
    namespace HitGroup
    {
        static const UINT Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };

        static const UINT GeometryStride = RayType::Count;
    }

    namespace MissShader
    {
        static const UINT Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_BEGIN(16)

// ----------------------------------------------------------------------------------------------------------------------------

struct RaytracingGlobalCB
{
    XMFLOAT4    CameraPosition;
    XMFLOAT4X4  InverseTransposeViewProjectionMatrix;
    XMFLOAT2    OutputResolution;

    float       AORadius;
    int         FrameCount;
    int         NumRays;
    int         AccumCount;
    int         HitProgramCount;

    int         AOMissIndex;
    int         AOHitGroupIndex;
    int         DirectLightingMissIndex;
    int         DirectLightingHitGroupIndex;
    int         IndirectLightingMissIndex;
    int         IndirectLightingHitGroupIndex;
};

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_END

#endif
