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

#include "ShaderStructs.hlsl"
#include "Lighting.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 PositionVS : TEXCOORD0;
    float3 NormalVS   : TEXCOORD1;
    float2 TexCoord   : TEXCOORD2;
    float4 SMCoord    : TEXCOORD3;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct MRT
{
    float4 Position : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 TexCoord : SV_Target2;
    float4 Diffuse  : SV_Target3;
};

// ----------------------------------------------------------------------------------------------------------------------------

SamplerState LinearRepeatSampler    : register(s0);
Texture2D    DiffuseTexture         : register(t0);

// ----------------------------------------------------------------------------------------------------------------------------

MRT main(PixelShaderInput IN)
{
    MRT mrt;
    mrt.Position = IN.PositionVS;
    mrt.Normal   = float4(IN.NormalVS, 1);
    mrt.TexCoord = float4(IN.TexCoord, 0, 0);
    mrt.Diffuse  = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord);

    return mrt;
}