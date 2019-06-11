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

ConstantBuffer<DenoiseConstantBuffer>   PassCB                  : register(b0, space0);

RWTexture2D<float4>                     PrevReprojMoments       : register(u0, space0);
RWTexture2D<float4>                     CurrReprojMoments       : register(u1, space0);
RWTexture2D<float>                      PrevReprojHistoryLength : register(u2, space0);
RWTexture2D<float>                      CurrReprojHistoryLength : register(u3, space0);
RWTexture2D<float4>                     PrevReprojDirect        : register(u4, space0);
RWTexture2D<float4>                     CurrReprojDirect        : register(u5, space0);
RWTexture2D<float4>                     PrevReprojIndirect      : register(u6, space0);
RWTexture2D<float4>                     CurrReprojIndirect      : register(u7, space0);

Texture2D                               Motion                  : register(t0, space0);
Texture2D                               PrevLinearZ             : register(t1, space0);
Texture2D                               CurrLinearZ             : register(t2, space0);
Texture2D                               SourceDirect            : register(t3, space0);
Texture2D                               SourceIndirect          : register(t4, space0);

// ----------------------------------------------------------------------------------------------------------------------------

inline bool isReprojectionValid(int2 coord, int2 imageDim, float currentZ, float previousZ, float fwidthZ, float3 currentNormal, float3 prevNormal, float fwidthNormal)
{
    bool valid = true;

    // Check whether reprojected pixel is inside of the screen
    if (any(lessThan(coord, int2(1, 1))) || any(greaterThan(coord, imageDim - int2(1, 1))))
    {
        valid = false;
    }

    // Check if deviation of depths is acceptable
    if (abs(previousZ - currentZ) / (fwidthZ + 1e-4) > 2.0)
    {
        valid = false;
    }

    // Check normals for compatibility
    if (distance(currentNormal, prevNormal) / (fwidthNormal + 1e-2) > 16.0)
    {
        valid = false;
    }

    return valid;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline bool loadPrevData(int2 ipos, out float4 prevDirect, out float4 prevIndirect, out float4 prevMoments, out float historyLength)
{
    const float2  imageDim  = float2(getTextureDims(SourceDirect, 0));
	const float4  motion    = Motion[ipos];                                                 // xy = motion, z = length(fwidth(pos)), w = length(fwidth(normal))
    const int2    iposPrev  = int2(float2(ipos) + motion.xy * imageDim + float2(0.5, 0.5)); // +0.5 to account for texel center offset
	const float4  depth     = CurrLinearZ[ipos];                                            // stores: Z, fwidth(z), z_prev
    const float3  normal    = octToDir(asuint(depth.w));
    const float2  posPrev   = ipos + motion.xy * imageDim;
    const int2    offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };

    prevDirect   = float4(0,0,0,0);
    prevIndirect = float4(0,0,0,0);
    prevMoments  = float4(0,0,0,0);
    
    // Check for all 4 taps of the bilinear filter for validity
    bool v[4];
	bool valid = false;
    for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
    { 
        int2   loc        = int2(posPrev) + offset[sampleIdx];
        float4 depthPrev  = PrevLinearZ[loc];
        float3 normalPrev = octToDir(asuint(depthPrev.w));

        v[sampleIdx] = isReprojectionValid(iposPrev, imageDim, depth.z, depthPrev.x, depth.y, normal, normalPrev, motion.w);

        valid = valid || v[sampleIdx];
    }    

    if (valid) 
    {
        float sumw = 0;
        float x    = frac(posPrev.x);
        float y    = frac(posPrev.y);

        // Bilinear weights
        float w[4] = { (1 - x) * (1 - y), 
                            x  * (1 - y), 
                       (1 - x) *      y,
                            x  *      y };

        prevDirect   = float4(0,0,0,0);
        prevIndirect = float4(0,0,0,0);
        prevMoments  = float4(0,0,0,0);

        // Perform the actual bilinear interpolation
        for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
        {
            int2 loc = int2(posPrev) + offset[sampleIdx];            
            if (v[sampleIdx])
            {
                prevDirect   += w[sampleIdx] * PrevReprojDirect[loc];
                prevIndirect += w[sampleIdx] * PrevReprojIndirect[loc];
                prevMoments  += w[sampleIdx] * PrevReprojMoments[loc];
                sumw         += w[sampleIdx];
            }
        }

		// Redistribute weights in case not all taps were used
		valid        = (sumw >= 0.01);
		prevDirect   = valid ? prevDirect / sumw   : float4(0, 0, 0, 0);
		prevIndirect = valid ? prevIndirect / sumw : float4(0, 0, 0, 0);
		prevMoments  = valid ? prevMoments / sumw  : float4(0, 0, 0, 0);
    }

    // Perform cross-bilateral filter in the hope to find some suitable samples somewhere
    if (!valid) 
    {
        float cnt = 0.0;

        // This code performs a binary descision for each tap of the cross-bilateral filter
        const int radius = 1;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                const int2  p            = iposPrev + int2(xx, yy);
                float4      depthFilter  = PrevLinearZ[p];
				float3      normalFilter = octToDir(asuint(depthFilter.w));

                if (isReprojectionValid(iposPrev, imageDim, depth.z, depthFilter.x, depth.y, normal, normalFilter, motion.w) )
                {
					prevDirect   += PrevReprojDirect[p];
                    prevIndirect += PrevReprojIndirect[p];
					prevMoments  += PrevReprojMoments[p];
                    cnt += 1.0;
                }
            }
        }
        if (cnt > 0)
        {
            valid         = true;
            prevDirect   /= cnt;
            prevIndirect /= cnt;
            prevMoments  /= cnt;
        }
    }

    if (valid)
    {
        // crude, fixme
        historyLength = PrevReprojHistoryLength[iposPrev];
    }
    else
    {
        prevDirect    = float4(0,0,0,0);
        prevIndirect  = float4(0,0,0,0);
        prevMoments   = float4(0,0,0,0);
        historyLength = 0;
    }

    return valid;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline void loadCurrData(int2 fragCoord, out float3 direct, out float3 indirect)
{
	direct   = SourceDirect[fragCoord].rgb;
    indirect = SourceIndirect[fragCoord].rgb;
}

// ----------------------------------------------------------------------------------------------------------------------------

[numthreads(COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y, COMPUTE_THREAD_GROUPSIZE_Z)]
void main(uint2 coord : SV_DispatchThreadID)
{
    // Load current
    float3 direct, indirect;
    loadCurrData(coord, direct, indirect);

    // Load previous
    float4  prevDirect, prevIndirect, prevMoments;
    float   historyLength;
    bool    success;
	success       = loadPrevData(coord, prevDirect, prevIndirect, prevMoments, historyLength);
	historyLength = min( 32.0f, success ? historyLength + 1.0f : 1.0f );

    // This adjusts the alpha for the case where insufficient history is available.
    // It boosts the temporal accumulation to give the samples equal weights in the beginning.
    const float alpha        = success ? max(PassCB.Alpha,        1.0 / historyLength) : 1.0;
    const float alphaMoments = success ? max(PassCB.MomentsAlpha, 1.0 / historyLength) : 1.0;

    // Compute first two moments of luminance
    float4 moments;
    float2 variance;
    moments.r = luminance(direct);
    moments.b = luminance(indirect);
    moments.g = moments.r * moments.r;
    moments.a = moments.b * moments.b;

    // Temporal integration of the moments
    moments  = lerp(prevMoments, moments, alphaMoments);
    variance = max(float2(0, 0), moments.ga - moments.rb * moments.rb);

    // Temporal integration of direct and indirect
    float4 temporalDirect   = lerp(prevDirect, float4(direct, 0), alpha);
    float4 temporalIndirect = lerp(prevIndirect, float4(indirect, 0), alpha);

    // Write out results
    CurrReprojMoments[coord]        = moments;
    CurrReprojHistoryLength[coord]  = historyLength;
    CurrReprojDirect[coord]         = float4(temporalDirect.rgb, variance.r);
    CurrReprojIndirect[coord]       = float4(temporalIndirect.rgb, variance.g);
}
