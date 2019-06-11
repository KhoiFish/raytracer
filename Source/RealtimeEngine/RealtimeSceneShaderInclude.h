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

#define RAYTRACING_MAX_RAY_RECURSION_DEPTH  10
#define RAYTRACING_MAX_NUM_LIGHTS           8
#define RAYTRACING_MAX_NUM_MATERIALS        1024
#define RAYTRACING_MAX_NUM_DIFFUSETEXTURES  1024
#define RAYTRACING_MAX_NUM_VERTEXBUFFERS    2048
#define RAYTRACING_MAX_NUM_INDEXBUFFERS     2048

#define RAYTRACING_INSTANCEMASK_ALL         (0xFF)
#define RAYTRACING_INSTANCEMASK_OPAQUE      (1 << 0)
#define RAYTRACING_INSTANCEMASK_AREALIGHT   (1 << 1)

// These would be nice as an enum, but alas not supported under shader model 5.0
#define RenderMaterialType_Light            0
#define RenderMaterialType_Lambert          1
#define RenderMaterialType_Metal            2
#define RenderMaterialType_MDielectric      3

// ----------------------------------------------------------------------------------------------------------------------------

struct RealtimeSceneVertex
{
    XMFLOAT3    Position;
    XMFLOAT3    Normal;
    XMFLOAT2    TexCoord;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderNodeInstanceData
{
    XMMATRIX    WorldMatrix;
    XMMATRIX    PrevWorldMatrix;
    int         LightIndex;
    UINT        InstanceId;
    UINT        Padding0;
    UINT        Padding1;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMaterial
{
    XMFLOAT4    Emissive;
    XMFLOAT4    Diffuse;
    UINT        Type;
    UINT        DiffuseTextureId;
    float       Fuzz;
    float       ReflIndex;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RealtimeAreaLight
{
    XMFLOAT4    NormalWS;   
    XMFLOAT4    Color;
    float       AreaCoverage;
    float       PlaneA0, PlaneA1, PlaneB0;
    float       PlaneB1, PlaneC0, PlaneC1;
    UINT        Padding0;
};

// ----------------------------------------------------------------------------------------------------------------------------

#endif