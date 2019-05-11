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

struct RenderMatrices
{
    float4x4 ModelMatrix;
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ModelViewMatrix;
    float4x4 InverseTransposeModelViewMatrix;
    float4x4 ModelViewProjectionMatrix;
    float4x4 ShadowViewProj;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMaterial
{
    float4 Emissive;
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float  SpecularPower;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct SpotLight
{
    float4 PositionWS;
    float4 PositionVS;
    float4 DirectionWS;
    float4 DirectionVS;
    float4 LookAtWS;
    float4 UpWS;
    float4 SmapWS;
    float4 Color;
    float  SpotAngle;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    float  QuadraticAttenuation;
    int    ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct DirLight
{
    float4 DirectionWS;
    float4 DirectionVS;
    float4 Color;
    int    ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct GlobalData
{
    int   NumSpotLights;
    int   NumDirLights;
    float ShadowmapDepth;
};
