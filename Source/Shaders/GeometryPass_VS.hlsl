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
#include "../RealtimeEngine/RenderSceneShader.h"

// ----------------------------------------------------------------------------------------------------------------------------

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionVS : TEXCOORD0;
    float3 NormalVS   : TEXCOORD1;
    float2 TexCoord   : TEXCOORD2;
    float4 Position   : SV_Position;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<SceneConstantBuffer> MatricesCB : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    float4 worldPos = mul(float4(IN.Position, 1.0f), MatricesCB.ModelMatrix);

    OUT.Position   = mul(float4(IN.Position, 1.0f), MatricesCB.ModelViewProjectionMatrix);
    OUT.PositionVS = mul(float4(IN.Position, 1.0f), MatricesCB.ModelViewMatrix);
    OUT.NormalVS   = mul(IN.Normal, (float3x3)MatricesCB.InverseTransposeModelViewMatrix);
    OUT.TexCoord   = IN.TexCoord;

    return OUT;
}
