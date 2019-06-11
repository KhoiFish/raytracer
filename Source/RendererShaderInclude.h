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

#ifndef RENDERERSHADERINCLUDE_H
#define RENDERERSHADERINCLUDE_H

#include "RealtimeEngine/ShaderCompat.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define COMPUTE_THREAD_GROUPSIZE_X  128
#define COMPUTE_THREAD_GROUPSIZE_Y  1
#define COMPUTE_THREAD_GROUPSIZE_Z  1

// ----------------------------------------------------------------------------------------------------------------------------

// Align the following structs to 16 bytes
ALIGN_BEGIN(16)

// ----------------------------------------------------------------------------------------------------------------------------

struct SceneConstantBuffer
{
    XMFLOAT4    CameraPosition;
    XMFLOAT4    OutputSize;
    XMFLOAT4X4  ViewMatrix;
    XMFLOAT4X4  ProjectionMatrix;
    XMFLOAT4X4  ViewProjectionMatrix;
    XMFLOAT4X4  PrevViewProjectionMatrix;
    XMFLOAT4X4  InverseViewProjectionMatrix;
    XMFLOAT4X4  InverseTransposeViewProjectionMatrix;
    XMFLOAT2    CameraJitter;
    float       FarClipDist;
    float       Padding0;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct CompositeConstantBuffer
{
    XMFLOAT4    TextureMultipliers[9];
    XMFLOAT4    CompositeMultipliers[6];
    XMFLOAT2    DirectIndirectLightMult;
    float       CpuNormalizeFactor;
    float       AccumCount;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct DenoiseRootConstants
{
    UINT StepSize;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct DenoiseConstantBuffer
{
    float Alpha;
    float MomentsAlpha;
    float PhiColor;
    float PhiNormal;
};

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_END

#endif