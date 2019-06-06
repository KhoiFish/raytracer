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

struct HaltonState
{
    UINT Dimension;
    UINT SequenceIndex;
};

struct LightingResult
{
    XMFLOAT3 Result;
    XMFLOAT3 Albedo;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct ShadowRayPayload
{
    float Value;
};

struct AreaLightRayPayload
{
    XMFLOAT3 LightColor;
    int      LightIndex;
};

struct ShadeRayPayload
{
    LightingResult  Results;
    UINT            RndSeed;
    UINT            RayDepth;
    HaltonState     HState;
};

// Set this to the largest payload struct from above
#define RAYTRACER_MAX_PAYLOAD_SIZE  sizeof(ShadeRayPayload)

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_BEGIN(16)
struct RaytracingGlobalCB
{
    XMFLOAT4    CameraPosition;
    XMFLOAT4    CameraTarget;
    XMFLOAT4X4  InverseTransposeViewProjectionMatrix;
    XMFLOAT2    OutputResolution;

    float       AORadius;
    int         FrameCount;
    int         RaysPerPixel;
    int         AccumCount;
    int         NumLights;
    int         MaxRayDepth;
    int         NumHitPrograms;

    int         AOMissIndex;
    int         AOHitGroupIndex;
    int         AreaLightMissIndex;
    int         AreaLightHitGroupIndex;
    int         ShadeMissIndex;
    int         ShadeHitGroupIndex;
};
ALIGN_END

#endif
