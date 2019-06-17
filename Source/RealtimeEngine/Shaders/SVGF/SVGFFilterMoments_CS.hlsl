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
#include "../../Renderer/RendererShaderInclude.h"
#include "SVGFCommon.h"

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<DenoiseConstantBuffer>   PassCB                      : register(b0, space0);

RWTexture2D<float4>                     CurrReprojMoments           : register(u1,  space0);
RWTexture2D<float>                      CurrReprojHistoryLength     : register(u3,  space0);
RWTexture2D<float4>                     CurrReprojDirect            : register(u5,  space0);
RWTexture2D<float4>                     CurrReprojIndirect          : register(u7,  space0);

RWTexture2D<float4>                     CurrDenoiseOutputDirect     : register(u9,  space0);
RWTexture2D<float4>                     CurrDenoiseOutputIndirect   : register(u11, space0);

Texture2D                               Compact                     : register(t5,  space0);

// ----------------------------------------------------------------------------------------------------------------------------

[numthreads(COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y, COMPUTE_THREAD_GROUPSIZE_Z)]
void main(uint2 ipos : SV_DispatchThreadID)
{
	float h          = CurrReprojHistoryLength[ipos].r;
    int2  screenSize = getTextureDims(CurrReprojDirect, 0);

    // Not enough temporal history available
    if (h < 4.0)
    {
        float        sumWDirect      = 0.0;
        float        sumWIndirect    = 0.0;
        float3       sumDirect       = float3(0.0, 0.0, 0.0);
        float3       sumIndirect     = float3(0.0, 0.0, 0.0);
        float4       sumMoments      = float4(0.0, 0.0, 0.0, 0.0);
        const float4 directCenter    = CurrReprojDirect[ipos];
        const float4 indirectCenter  = CurrReprojIndirect[ipos];
		const float  lDirectCenter   = luminance(directCenter.rgb);
        const float  lIndirectCenter = luminance(indirectCenter.rgb);

        float3 normalCenter;
        float2 zCenter;
        fetchNormalAndLinearZ(Compact, ipos, normalCenter, zCenter);

        // Current pixel does not a valid depth => must be envmap => do nothing
        if (zCenter.x < 0)
        {
            CurrDenoiseOutputDirect[ipos]   = directCenter;
            CurrDenoiseOutputIndirect[ipos] = indirectCenter;

            return;
        }

        // Compute first and second moment spatially. This code also applies cross-bilateral filtering on the input color samples
        const float phiNormal    = PassCB.PhiNormal;
        const float phiLDirect   = PassCB.PhiColor;
        const float phiLIndirect = PassCB.PhiColor;
        const float phiDepth     = max(zCenter.y, 1e-8) * 3.0f;
        const int   radius       = 3;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                const int2  p         = ipos + int2(xx, yy);
                const bool  inside    = all(greaterThanEqual(p, int2(0,0))) && all(lessThan(p, screenSize));
                const bool  samePixel = (xx==0) && (yy==0);
                const float kernel    = 1.0f;

                if (inside)
                {
                    const float3 directP     = CurrReprojDirect[p].rgb;
                    const float3 indirectP   = CurrReprojIndirect[p].rgb;
                    const float4 momentsP    = CurrReprojMoments[p];
                    const float  lDirectP    = luminance(directP.rgb);
                    const float  lIndirectP  = luminance(indirectP.rgb);

                    float3 normalP;
                    float2 zP;
                    fetchNormalAndLinearZ(Compact, p, normalP, zP);

                    const float2 w = computeWeight(
                        zCenter.x, zP.x, phiDepth * length(float2(xx, yy)),
						normalCenter, normalP, phiNormal,
                        lDirectCenter, lDirectP, phiLDirect,
                        lIndirectCenter, lIndirectP, phiLIndirect);

                    const float wDirect   = w.x;
                    const float wIndirect = w.y;

                    sumWDirect    += wDirect;
                    sumDirect     += directP * wDirect;

                    sumWIndirect  += wIndirect;
                    sumIndirect   += indirectP * wIndirect;

					sumMoments    += momentsP * float4(wDirect.xx, wIndirect.xx);
                }
            }
        }

		// Clamp sums to >0 to avoid NaNs.
		sumWDirect   = max(sumWDirect, 1e-6f);
		sumWIndirect = max(sumWIndirect, 1e-6f);

        sumDirect   /= sumWDirect;
        sumIndirect /= sumWIndirect;
        sumMoments  /= float4(sumWDirect.xx, sumWIndirect.xx);

        // Compute variance for direct and indirect illumination using first and second moments
        float2 variance = sumMoments.ga - sumMoments.rb * sumMoments.rb;

        // Give the variance a boost for the first frames
        variance *= 4.0 / h;

        CurrDenoiseOutputDirect[ipos]   = float4(sumDirect, variance.r);
        CurrDenoiseOutputIndirect[ipos] = float4(sumIndirect, variance.g);

        return;
    }
    else
    {
        // Do nothing, pass data unmodified
        CurrDenoiseOutputDirect[ipos]   = CurrReprojDirect[ipos];
        CurrDenoiseOutputIndirect[ipos] = CurrReprojIndirect[ipos];

        return;
    }
}
