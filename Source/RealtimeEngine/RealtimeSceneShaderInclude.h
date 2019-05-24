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

#ifndef REALTIMESCENESHADERINCLUDE_H
#define REALTIMESCENESHADERINCLUDE_H

#include "ShaderCompat.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define RAYTRACING_MAX_NUM_MATERIALS        1024
#define RAYTRACING_MAX_NUM_LIGHTS           8

#define RAYTRACING_INSTANCEMASK_ALL         (0xFF)
#define RAYTRACING_INSTANCEMASK_OPAQUE      (1 << 0)
#define RAYTRACING_INSTANCEMASK_AREALIGHT   (1 << 1)

// ----------------------------------------------------------------------------------------------------------------------------

//ALIGN_BEGIN(16)

// ----------------------------------------------------------------------------------------------------------------------------

struct RealtimeSceneVertex
{
    XMFLOAT3   Position;
    XMFLOAT3   Normal;
    XMFLOAT2   TexCoord;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderNodeInstanceData
{
    XMMATRIX  WorldMatrix;
    XMFLOAT4  AverageAlbedo;
    UINT      InstanceId;
    int       LightIndex;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMaterial
{
    XMFLOAT4   Emissive;
    XMFLOAT4   Ambient;
    XMFLOAT4   Diffuse;
    XMFLOAT4   Specular;
    float      SpecularPower;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RealtimeAreaLight
{
    XMFLOAT4    NormalWS;   
    XMFLOAT4    Color;
    float       AreaCoverage;
    float       PlaneA0, PlaneA1, PlaneB0, PlaneB1, PlaneC0, PlaneC1;
};


//ALIGN_END

#endif