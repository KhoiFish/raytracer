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
#include "../RealtimeSceneShaderInclude.h"
#include "../Renderer/RendererShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionWS     : TEXCOORD0;
    float3 NormalWS       : TEXCOORD1;
    float2 TexCoord       : TEXCOORD2;
    float  LinearDepth    : TEXCOORD3;
    float3 NormalOS       : TEXCOORD4;
    float4 PrevPositionHS : TEXCOORD5;
    float4 PositionHS     : SV_Position;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct MRT
{
    float4 Position         : SV_Target0;
    float4 Normal           : SV_Target1;
    float4 TexCoordAndDepth : SV_Target2;
    float4 Diffuse          : SV_Target3;
    float4 SVGFLinearZ      : SV_Target4;
    float4 SVGFMoVec        : SV_Target6;
    float4 SVGFCompact      : SV_Target7;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState                            LinearRepeatSampler : register(s0, space0);
SamplerState                            AnisoRepeatSampler  : register(s1, space0);

Texture2D                               DiffuseTexture      : register(t3, space1);

ConstantBuffer<SceneConstantBuffer>     SceneCb             : register(b0, space0);
ConstantBuffer<RenderNodeInstanceData>  InstanceCb          : register(b1, space0);
ConstantBuffer<RenderMaterial>          MaterialCb          : register(b2, space0);

// ----------------------------------------------------------------------------------------------------------------------------

// A simple utility to convert a float to a 2-component octohedral representation packed into one uint
uint dirToOct(float3 normal)
{
    float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
    float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p) * 2.0 - (float2)(1.0));

    return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

// ----------------------------------------------------------------------------------------------------------------------------

// Take current clip position, last frame pixel position and compute a motion vector
float2 calcMotionVector(float4 prevClipPos, float2 currentPixelPos, float2 invFrameSize)
{
    float2 prevPosNDC = (prevClipPos.xy / prevClipPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);
    float2 motionVec  = prevPosNDC - (currentPixelPos * invFrameSize);

    // Guard against inf/nan due to projection by w <= 0.
    const float epsilon = 1e-5f;
    motionVec = (prevClipPos.w < epsilon) ? float2(0, 0) : motionVec;

    return motionVec;
}

// ----------------------------------------------------------------------------------------------------------------------------

MRT main(PixelShaderInput IN)
{
    // Linear Z buffer
    float  linearZ          = IN.PositionHS.z * IN.PositionHS.w;
    float  maxChangeZ       = max(abs(ddx(linearZ)), abs(ddy(linearZ)));
    float  objNorm          = asfloat(dirToOct(normalize(IN.NormalOS)));
    float4 svgfLinearZOut   = float4(linearZ, maxChangeZ, IN.PrevPositionHS.z, objNorm);

    // Motion vector buffer
    float2 svgfMotionVec    = calcMotionVector(IN.PrevPositionHS, IN.PositionHS.xy, SceneCb.OutputSize.zw) + float2(SceneCb.CameraJitter.x, -SceneCb.CameraJitter.y);
    float2 posNormFWidth    = float2(length(fwidth(IN.PositionWS)), length(fwidth(IN.NormalWS)));
    float4 svgfMotionVecOut = float4(svgfMotionVec, posNormFWidth);

    // Have lights be pass through like the background
    float4 outPos = (InstanceCb.LightIndex < 0) 
                        ? float4(IN.PositionWS.xyz, 1) 
                        : float4(0, 0, 0, 0);

    // Write out MRT
    MRT mrt;
    mrt.Position         = outPos;
    mrt.Normal           = float4(IN.NormalWS, InstanceCb.InstanceId);
    mrt.TexCoordAndDepth = float4(IN.TexCoord, IN.LinearDepth, 0);
    mrt.Diffuse          = DiffuseTexture.Sample(AnisoRepeatSampler, IN.TexCoord);
    mrt.SVGFLinearZ      = svgfLinearZOut;
    mrt.SVGFMoVec        = svgfMotionVecOut;
    mrt.SVGFCompact      = float4(asfloat(dirToOct(IN.NormalWS)), linearZ, maxChangeZ, 0.0f);

    return mrt;
}
