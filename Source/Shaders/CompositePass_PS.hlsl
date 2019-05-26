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
    float2 TexCoord : TEXCOORD;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<CompositeConstantBuffer> CompositeCB             : register(b2, space0);

Texture2D                               DirectLightAO           : register(t0, space0);
Texture2D                               IndirectLight           : register(t1, space0);
Texture2D                               CpuResultsTex           : register(t2, space0);
Texture2D                               PositionsTexture        : register(t3, space0);
Texture2D                               NormalsTexture          : register(t4, space0);
Texture2D                               TexCoordsTexture        : register(t5, space0);
Texture2D                               DiffuseTexture          : register(t6, space0);

SamplerState                            LinearRepeatSampler     : register(s0, space0);
SamplerState                            AnisoRepeatSampler      : register(s1, space0);

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    // Direct lighting and ao stored in the same buffer
    float4 directLightAO = DirectLightAO.Sample(LinearRepeatSampler, IN.TexCoord);

    // Get all the buffer contributions
    float4 directLight   = float4(directLightAO.rgb, 1)                           * CompositeCB.TextureMultipliers[1] * CompositeCB.DirectIndirectLightMult.x;
    float4 indirectLight = IndirectLight.Sample(AnisoRepeatSampler, IN.TexCoord)  * CompositeCB.TextureMultipliers[2] * CompositeCB.DirectIndirectLightMult.y;
    float4 ao            = directLightAO.a                                        * CompositeCB.TextureMultipliers[3];
    float4 diffuse       = DiffuseTexture.Sample(AnisoRepeatSampler, IN.TexCoord) * CompositeCB.TextureMultipliers[4];
    float4 cpuRT         = CpuResultsTex.Sample(AnisoRepeatSampler, IN.TexCoord)  * CompositeCB.TextureMultipliers[5];

    // Compute semi-final colors
    float4 composited    = (directLight + indirectLight) * ao;
    float4 selected      = (directLight + indirectLight + ao + diffuse + cpuRT);

    // Compute final color
    float4 finalCol = (composited * CompositeCB.CompositeMultipliers[0]) + (selected * CompositeCB.CompositeMultipliers[1]);

    return finalCol;
}
