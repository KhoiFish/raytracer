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
    float2 TexCoord : TEXCOORD;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct MRT
{
    float4 BackBuffer       : SV_Target0;
    float4 CompositeBuffer  : SV_Target1;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<CompositeConstantBuffer> CompositeCB             : register(b3, space0);

Texture2D                               DirectResult            : register(t0, space0);
Texture2D                               DirectAlbedo            : register(t1, space0);
Texture2D                               IndirectResult          : register(t2, space0);
Texture2D                               IndirectAlbedo          : register(t3, space0);
Texture2D                               DenoisedDirect          : register(t4, space0);
Texture2D                               DenoisedIndirect        : register(t5, space0);
Texture2D                               CpuResultsTex           : register(t6, space0);
Texture2D                               PrevResultsTexture      : register(t7, space0);

Texture2D                               PositionsTexture        : register(t0, space1);
Texture2D                               NormalsTexture          : register(t1, space1);
Texture2D                               TexCoordsTexture        : register(t2, space1);
Texture2D                               DiffuseTexture          : register(t3, space1);

SamplerState                            AnisoRepeatSampler      : register(s1, space0);

// ----------------------------------------------------------------------------------------------------------------------------

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;

    return a / b;
}

// From https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// Originally by Stephen Hill (@self_shadow)
float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

// ----------------------------------------------------------------------------------------------------------------------------

MRT main(PixelShaderInput IN)
{
    // Direct lighting and ao stored in the same buffer
    float4 rawDirect        = DirectResult.Sample(AnisoRepeatSampler, IN.TexCoord)     * DirectAlbedo.Sample(AnisoRepeatSampler, IN.TexCoord);
    float4 rawIndirect      = IndirectResult.Sample(AnisoRepeatSampler, IN.TexCoord)   * IndirectAlbedo.Sample(AnisoRepeatSampler, IN.TexCoord);
    float4 denoisedDirect   = DenoisedDirect.Sample(AnisoRepeatSampler, IN.TexCoord)   * DirectAlbedo.Sample(AnisoRepeatSampler, IN.TexCoord);
    float4 denoisedIndirect = DenoisedIndirect.Sample(AnisoRepeatSampler, IN.TexCoord) * IndirectAlbedo.Sample(AnisoRepeatSampler, IN.TexCoord);
    float4 computedDirect   = (rawDirect * CompositeCB.CompositeMultipliers[3])   + (denoisedDirect * (float4(1, 1, 1, 1) - CompositeCB.CompositeMultipliers[3]));
    float4 computedIndirect = (rawIndirect * CompositeCB.CompositeMultipliers[3]) + (denoisedIndirect * (float4(1, 1, 1, 1) - CompositeCB.CompositeMultipliers[3]));

    // Get all the buffer contributions
    float4 directLight   = computedDirect                                           * CompositeCB.TextureMultipliers[1] * CompositeCB.DirectIndirectLightMult.x;
    float4 indirectLight = computedIndirect                                         * CompositeCB.TextureMultipliers[2] * CompositeCB.DirectIndirectLightMult.y;
    float4 cpuRT         = CpuResultsTex.Sample(AnisoRepeatSampler, IN.TexCoord)    * CompositeCB.TextureMultipliers[3];
    float4 positions     = PositionsTexture.Sample(AnisoRepeatSampler, IN.TexCoord) * CompositeCB.TextureMultipliers[4];
    float4 normals       = NormalsTexture.Sample(AnisoRepeatSampler, IN.TexCoord)   * CompositeCB.TextureMultipliers[5];
    float4 texCoord      = TexCoordsTexture.Sample(AnisoRepeatSampler, IN.TexCoord) * CompositeCB.TextureMultipliers[6];
    float4 diffuse       = DiffuseTexture.Sample(AnisoRepeatSampler, IN.TexCoord)   * CompositeCB.TextureMultipliers[7];

    // Normalize cpu raytraced image
    cpuRT *= CompositeCB.CpuNormalizeFactor;

    // Compute semi-final colors
    float4 composited    = (directLight + indirectLight);
    float4 selected      = (directLight + indirectLight + cpuRT + positions + normals + texCoord + diffuse);
    float4 noRaytracing  = diffuse;

    // Compute final color
    float4 compositeColor = (composited * CompositeCB.CompositeMultipliers[0]) + (selected * CompositeCB.CompositeMultipliers[1]) + (noRaytracing * CompositeCB.CompositeMultipliers[5]);

    // Tone map
    float4 toneMapped = (float4(ACESFitted(compositeColor.rgb), 1) * CompositeCB.CompositeMultipliers[2]) + (compositeColor * (float4(1, 1, 1, 1) - CompositeCB.CompositeMultipliers[2]));

    // Temporal accumulation after we get more than 1 frame
    if (CompositeCB.AccumCount > 0)
    {
        toneMapped = lerp(PrevResultsTexture.Sample(AnisoRepeatSampler, IN.TexCoord), toneMapped, (1.0f / CompositeCB.AccumCount));
    }

    // Choose between filtered or unfiltered output
    float4 finalColor = (compositeColor * CompositeCB.CompositeMultipliers[4]) + (toneMapped * (float4(1, 1, 1, 1) - CompositeCB.CompositeMultipliers[4]));

    // Write out results
    MRT mrt;
    mrt.BackBuffer      = finalColor;
    mrt.CompositeBuffer = finalColor;

    return mrt;
}
