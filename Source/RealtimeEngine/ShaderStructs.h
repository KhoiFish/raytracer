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

#ifndef SHADERSTRUCTS_H
#define SHADERSTRUCTS_H

// ----------------------------------------------------------------------------------------------------------------------------

#ifdef HLSL
    typedef float2      XMFLOAT2;
    typedef float3      XMFLOAT3;
    typedef float4      XMFLOAT4;
    typedef float4      XMVECTOR;
    typedef float4x4    XMMATRIX;
    typedef float4x4    XMFLOAT4X4;
    typedef uint        UINT;

    #define ALIGN_BEGIN(alignValue)
    #define ALIGN_END
#else
    #include <wrl.h>
    #include <d3dcompiler.h>
    #include <DirectXColors.h>
    #include <stdint.h>

    #define ALIGN_BEGIN(alignValue) __pragma(pack(push, alignValue))
    #define ALIGN_END               __pragma(pack(pop))

    using namespace DirectX;
#endif

// ----------------------------------------------------------------------------------------------------------------------------

#define MAX_RAY_RECURSION_DEPTH  3

// ----------------------------------------------------------------------------------------------------------------------------

static const XMFLOAT4   ChromiumReflectance = XMFLOAT4(0.549f, 0.556f, 0.554f, 1.0f);
static const XMFLOAT4   BackgroundColor     = XMFLOAT4(0.8f, 0.9f, 1.0f, 1.0f);
static const float      InShadowRadiance    = 0.35f;

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_BEGIN(16) // Align the following structs to 16 bytes

// ----------------------------------------------------------------------------------------------------------------------------

struct SceneConstantBuffer
{
    XMFLOAT4    CameraPosition;
    XMFLOAT4X4  ModelMatrix;
    XMFLOAT4X4  ViewMatrix;
    XMFLOAT4X4  ProjectionMatrix;
    XMFLOAT4X4  ModelViewMatrix;
    XMFLOAT4X4  ViewProjectionMatrix;
    XMFLOAT4X4  ModelViewProjectionMatrix;
    XMFLOAT4X4  InverseTransposeModelViewMatrix;
    XMFLOAT4X4  InverseViewProjectionMatrix;
    XMFLOAT4X4  ShadowViewProj;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct SceneVertex
{
    XMFLOAT3   Position;
    XMFLOAT3   Normal;
    XMFLOAT2   TexCoord;
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

struct SpotLight
{
    XMFLOAT4    PositionWS;
    XMFLOAT4    PositionVS;
    XMFLOAT4    DirectionWS;
    XMFLOAT4    DirectionVS;
    XMFLOAT4    LookAtWS;
    XMFLOAT4    UpWS;
    XMFLOAT4    SmapWS;
    XMFLOAT4    Color;
    float       SpotAngle;
    float       ConstantAttenuation;
    float       LinearAttenuation;
    float       QuadraticAttenuation;
    int         ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct DirLight
{
    XMFLOAT4    DirectionWS;
    XMFLOAT4    DirectionVS;
    XMFLOAT4    Color;
    int         ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct GlobalData
{
    int         NumSpotLights;
    int         NumDirLights;
    float       ShadowmapDepth;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RayPayload
{
    XMFLOAT4    Color;
    UINT        RecursionDepth;
};

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

struct PrimitiveConstantBuffer
{
    XMFLOAT4    Albedo;
    float       ReflectanceCoef;
    float       DiffuseCoef;
    float       SpecularCoef;
    float       SpecularPower;
    float       StepScale;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct PrimitiveInstanceConstantBuffer
{
    UINT        InstanceIndex;
    UINT        PrimitiveType;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct PrimitiveInstancePerFrameBuffer
{
    XMMATRIX    LocalSpaceToBottomLevelAS;
    XMMATRIX    BottomLevelASToLocalSpace;
};

// ----------------------------------------------------------------------------------------------------------------------------

ALIGN_END

#endif