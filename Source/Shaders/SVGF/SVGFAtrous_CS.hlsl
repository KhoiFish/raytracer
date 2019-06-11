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
// This file has been modified from https://research.nvidia.com/publication/2017-07_Spatiotemporal-Variance-Guided-Filtering%3A
//
// ----------------------------------------------------------------------------------------------------------------------------

#define HLSL
#include "../../RendererShaderInclude.h"
#include "SVGFCommon.h"

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<DenoiseConstantBuffer>   PassCB                      : register(b0,  space0);
ConstantBuffer<DenoiseRootConstants>    RootCB                      : register(b1,  space0);

RWTexture2D<float>                      CurrReprojHistoryLength     : register(u3,  space0);

RWTexture2D<float4>                     PrevDenoiseOutputDirect     : register(u8,  space0);
RWTexture2D<float4>                     CurrDenoiseOutputDirect     : register(u9,  space0);
RWTexture2D<float4>                     PrevDenoiseOutputIndirect   : register(u10, space0);
RWTexture2D<float4>                     CurrDenoiseOutputIndirect   : register(u11, space0);

Texture2D                               Compact                     : register(t5,  space0);

// ----------------------------------------------------------------------------------------------------------------------------

// Computes a 3x3 gaussian blur of the variance, centered around the current pixel
inline float2 computeVarianceCenter(int2 ipos, RWTexture2D<float4> sDirect, RWTexture2D<float4> sIndirect)
{
    float2      sum = float2(0.0, 0.0);
    const float kernel[2][2] = 
    {
        { 1.0 / 4.0, 1.0 / 8.0  },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    const int radius = 1;
    for (int yy = -radius; yy <= radius; yy++)
    {
        for (int xx = -radius; xx <= radius; xx++)
        {
            int2  p = ipos + int2(xx, yy);
            float k = kernel[abs(xx)][abs(yy)];

            sum.r += sDirect[p].a * k;
            sum.g += sIndirect[p].a * k;
        }
    }

    return sum;
}

// ----------------------------------------------------------------------------------------------------------------------------

[numthreads(COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y, COMPUTE_THREAD_GROUPSIZE_Z)]
void main(uint2 ipos : SV_DispatchThreadID)
{
    const int2   screenSize       = getTextureDims(PrevDenoiseOutputDirect, 0);
    const float  epsVariance      = 1e-10;
    const float  kernelWeights[3] = { 1.0, 2.0 / 3.0, 1.0 / 6.0 };
    const float4 directCenter     = PrevDenoiseOutputDirect[ipos];
    const float4 indirectCenter   = PrevDenoiseOutputIndirect[ipos];
    const float  lDirectCenter    = luminance(directCenter.rgb);
    const float  lIndirectCenter  = luminance(indirectCenter.rgb);

    // Variance for direct and indirect, filtered using 3x3 gaussin blur
    const float2 var = computeVarianceCenter(ipos, PrevDenoiseOutputDirect, PrevDenoiseOutputIndirect);

    // Number of temporally integrated pixels
    const float historyLength = CurrReprojHistoryLength[ipos];

    float3 normalCenter;
    float2 zCenter;
    fetchNormalAndLinearZ(Compact, ipos, normalCenter, zCenter);

    if (zCenter.x < 0)
    {
        // Not a valid depth => must be envmap => do not filter
        CurrDenoiseOutputDirect[ipos]   = directCenter;
        CurrDenoiseOutputIndirect[ipos] = indirectCenter;
        
        return;
    }

    const float phiLDirect   = PassCB.PhiColor * sqrt(max(0.0, epsVariance + var.r));
    const float phiLIndirect = PassCB.PhiColor * sqrt(max(0.0, epsVariance + var.g));
    const float phiDepth     = max(zCenter.y, 1e-8) * RootCB.StepSize;

    // Explicitly store/accumulate center pixel with weight 1 to prevent issues with the edge-stopping functions
    float  sumWDirect   = 1.0f;
    float  sumWIndirect = 1.0f;
    float4 sumDirect    = directCenter;
    float4 sumIndirect  = indirectCenter;
    for (int yy = -2; yy <= 2; yy++)
    {
        for (int xx = -2; xx <= 2; xx++)
        {
            const int2 p     = ipos + int2(xx, yy) * RootCB.StepSize;
            const bool inside = all(greaterThanEqual(p, int2(0,0))) && all(lessThan(p, screenSize));

            const float kernel = kernelWeights[abs(xx)] * kernelWeights[abs(yy)];

            // Skip center pixel, it is already accumulated
            if (inside && (xx != 0 || yy != 0))
            {
                const float4 directP     = PrevDenoiseOutputDirect[p];
                const float4 indirectP   = PrevDenoiseOutputIndirect[p];

                float3 normalP;
                float2 zP;
                fetchNormalAndLinearZ(Compact, p, normalP, zP);
                const float lDirectP   = luminance(directP.rgb);
                const float lIndirectP = luminance(indirectP.rgb);

                // Compute the edge-stopping functions
                const float2 w = computeWeight(
                    zCenter.x, zP.x, phiDepth * length(float2(xx, yy)),
					normalCenter, normalP, PassCB.PhiNormal,
                    lDirectCenter, lDirectP, phiLDirect,
                    lIndirectCenter, lIndirectP, phiLIndirect);

                const float wDirect = w.x * kernel;
                const float wIndirect = w.y * kernel;

                // Alpha channel contains the variance, therefore the weights need to be squared, see paper for the formula
                sumWDirect   += wDirect;
                sumDirect    += float4(wDirect.xxx, wDirect * wDirect) * directP;

                sumWIndirect += wIndirect;
                sumIndirect  += float4(wIndirect.xxx, wIndirect * wIndirect) * indirectP;
            }
        }
    }

    // Renormalization is different for variance, check paper for the formula
    CurrDenoiseOutputDirect[ipos]   = float4(sumDirect   / float4(sumWDirect.xxx,   sumWDirect   * sumWDirect  ));
    CurrDenoiseOutputIndirect[ipos] = float4(sumIndirect / float4(sumWIndirect.xxx, sumWIndirect * sumWIndirect));
}