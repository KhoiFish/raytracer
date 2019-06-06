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

// This file has been modified from https://research.nvidia.com/publication/2017-07_Spatiotemporal-Variance-Guided-Filtering%3A
#define HLSL
#include "../../RendererShaderInclude.h"
#include "SVGFCommon.h"
#include "SVGFPackNormal.h"
#include "SVGFEdgeStoppingFunctions.h"

// ----------------------------------------------------------------------------------------------------------------------------

enum LightingBufType
{
    LightingBufType_Prev = 0,
    LightingBufType_Current,
    LightingBufType_Output,
};

enum PingPong
{
    PingPong_Prev = 0,
    PingPong_Output
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<DenoiseConstantBuffer>   PassCB            : register(b0, space0);

RWTexture2D<float4>                     Moments[]         : register(u0, space0);
RWTexture2D<uint>                       HistoryLength[]   : register(u0, space1);
RWTexture2D<float4>                     Direct[]          : register(u0, space2);
RWTexture2D<float4>                     Indirect[]        : register(u0, space3);

Texture2D                               Motion            : register(t0, space0);
Texture2D                               PrevLinearZ       : register(t1, space0);
Texture2D                               CurrLinearZ       : register(t2, space0);

// ----------------------------------------------------------------------------------------------------------------------------

int2 getTextureRes(RWTexture2D<float4> tex, uint mip)
{
    uint w, h;
    tex.GetDimensions(w, h);

    return int2(w, h);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float3 demodulate(float3 x, float3 albedo)
{
    return x / max(albedo, float3(0.001f, 0.001f, 0.001f));
}

// ----------------------------------------------------------------------------------------------------------------------------

inline bool isReprojectionValid(int2 coord, float Z, float Zprev, float fwidthZ, float3 normal, float3 normalPrev, float fwidthNormal)
{
    const int2 imageDim = getTextureRes(Direct[LightingBufType_Current], 0);

    // Check whether reprojected pixel is inside of the screen
    if (any(lessThan(coord, int2(1,1))) || any(greaterThan(coord, imageDim - int2(1,1)))) return false;

    // Check if deviation of depths is acceptable
    if (abs(Zprev - Z) / (fwidthZ + 1e-4) > 2.0) return false;

    // Check normals for compatibility
    if (distance(normal, normalPrev) / (fwidthNormal + 1e-2) > 16.0) return false;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline bool loadPrevData(float2 fragCoord, out float4 prevDirect, out float4 prevIndirect, out float4 prevMoments, out float historyLength)
{
    const int2   ipos      = fragCoord;
    const float2 imageDim  = float2(getTextureRes(Direct[LightingBufType_Current], 0));
	float4       motion    = Motion[ipos];                                                 // xy = motion, z = length(fwidth(pos)), w = length(fwidth(normal))
    const int2   iposPrev  = int2(float2(ipos) + motion.xy * imageDim + float2(0.5,0.5));  // +0.5 to account for texel center offset
	float4       depth     = CurrLinearZ[ipos];                                            // stores: Z, fwidth(z), z_prev
    float3       normal    = octToDir(asuint(depth.w));
    const float2 posPrev   = floor(fragCoord.xy) + motion.xy * imageDim;
    int2         offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
    bool         v[4];

    prevDirect   = float4(0,0,0,0);
    prevIndirect = float4(0,0,0,0);
    prevMoments  = float4(0,0,0,0);
    
    // check for all 4 taps of the bilinear filter for validity
	bool valid = false;
    for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
    { 
        int2   loc        = int2(posPrev) + offset[sampleIdx];
        float4 depthPrev  = PrevLinearZ[loc];
        float3 normalPrev = octToDir(asuint(depthPrev.w));

        v[sampleIdx] = isReprojectionValid(iposPrev, depth.z, depthPrev.x, depth.y, normal, normalPrev, motion.w);

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
                prevDirect   += w[sampleIdx] * Direct[LightingBufType_Prev][loc];
                prevIndirect += w[sampleIdx] * Indirect[LightingBufType_Prev][loc];
                prevMoments  += w[sampleIdx] * Moments[PingPong_Prev][loc];
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
                int2   p            = iposPrev + int2(xx, yy);
                float4 depthFilter  = PrevLinearZ[p];
				float3 normalFilter = octToDir(asuint(depthFilter.w));

                if (isReprojectionValid(iposPrev, depth.z, depthFilter.x, depth.y, normal, normalFilter, motion.w) )
                {
					prevDirect   += Direct[LightingBufType_Prev][p];
                    prevIndirect += Indirect[LightingBufType_Prev][p];
					prevMoments  += Moments[PingPong_Prev][p];
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
        historyLength = HistoryLength[PingPong_Prev][iposPrev];
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
	direct   = Direct[LightingBufType_Current][fragCoord].rgb;
    indirect = Indirect[LightingBufType_Current][fragCoord].rgb;
}

// ----------------------------------------------------------------------------------------------------------------------------

[numthreads(COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y, COMPUTE_THREAD_GROUPSIZE_Z)]
void main(uint2 coord : SV_DispatchThreadID)
{
    float3  direct;
    float3  indirect;
    float   historyLength;
    float4  prevDirect;
    float4  prevIndirect;
    float4  prevMoments;
    bool    success;

    // Load current
    loadCurrData(coord, direct, indirect);

    // Load previous
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
    Moments[PingPong_Output][coord]         = moments;
    HistoryLength[PingPong_Output][coord]   = historyLength;
    Direct[LightingBufType_Output][coord]   = float4(temporalDirect.rgb, variance.r);
    Indirect[LightingBufType_Output][coord] = float4(temporalIndirect.rgb, variance.g);
}
