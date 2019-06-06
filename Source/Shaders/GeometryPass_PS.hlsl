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
#include "../RealtimeEngine/RealtimeSceneShaderInclude.h"
#include "../RendererShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionWS  : TEXCOORD0;
    float3 NormalWS    : TEXCOORD1;
    float2 TexCoord    : TEXCOORD2;
    float  LinearDepth : TEXCOORD3;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct MRT
{
    float4 Position         : SV_Target0;
    float4 Normal           : SV_Target1;
    float4 TexCoordAndDepth : SV_Target2;
    float4 Diffuse          : SV_Target3;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState                            LinearRepeatSampler : register(s0);
SamplerState                            AnisoRepeatSampler  : register(s1);

Texture2D                               DiffuseTexture      : register(t8);

ConstantBuffer<RenderNodeInstanceData>  InstanceCb          : register(b1);
ConstantBuffer<RenderMaterial>          MaterialCb          : register(b2);

// ----------------------------------------------------------------------------------------------------------------------------

MRT main(PixelShaderInput IN)
{
    // Have lights be pass through like the background
    float4 outPos = (InstanceCb.LightIndex < 0) 
                        ? float4(IN.PositionWS.xyz, 1) 
                        : float4(0, 0, 0, 0);

    MRT mrt;
    mrt.Position         = outPos;
    mrt.Normal           = float4(IN.NormalWS, InstanceCb.InstanceId);
    mrt.TexCoordAndDepth = float4(IN.TexCoord, IN.LinearDepth, 0);
    mrt.Diffuse          = DiffuseTexture.Sample(AnisoRepeatSampler, IN.TexCoord);

    return mrt;
}
