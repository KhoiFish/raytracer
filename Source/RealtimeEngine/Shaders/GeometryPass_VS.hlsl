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

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
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

ConstantBuffer<SceneConstantBuffer>     SceneCb     : register(b0);
ConstantBuffer<RenderNodeInstanceData>  InstanceCb  : register(b1);

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    float4 worldPos     = mul(float4(IN.Position, 1.0f), InstanceCb.WorldMatrix);
    float4 prevWorldPos = mul(float4(IN.Position, 1.0f), InstanceCb.PrevWorldMatrix);

    OUT.PositionWS     = worldPos;
    OUT.NormalWS       = mul(IN.Normal, (float3x3)InstanceCb.WorldMatrix);
    OUT.TexCoord       = IN.TexCoord;
    OUT.LinearDepth    = mul(float4(OUT.PositionWS.xyz, 1.0f), SceneCb.ViewMatrix).z / SceneCb.FarClipDist;
    OUT.NormalOS       = IN.Normal;
    OUT.PrevPositionHS = mul(prevWorldPos, SceneCb.PrevViewProjectionMatrix);
    OUT.PositionHS     = mul(worldPos, SceneCb.ViewProjectionMatrix);

    return OUT;
}
