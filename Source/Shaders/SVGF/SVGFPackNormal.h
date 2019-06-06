/**********************************************************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#  * Redistributions of code must retain the copyright notice, this list of conditions and the following disclaimer.
#  * Neither the name of NVIDIA CORPORATION nor the names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
# SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

#ifndef SVGF_PACK_NORMAL_H
#define SVGF_PACK_NORMAL_H

#include "SVGFCommon.h"

float3 octToDir(uint octo)
{
	float2 e = float2( f16tof32(octo & 0xFFFF), f16tof32((octo>>16) & 0xFFFF) ); 
	float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0.0)
		v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy)*2.0 - (float2)(1.0));
	return normalize(v);
}

uint dirToOct(float3 normal)
{
	float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
	float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p)*2.0 - (float2)(1.0));
	return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

void fetchNormalAndLinearZ(in Texture2D ndTexture, in int2 ipos, out float3 norm, out float2 zLinear)
{
	float4 nd = ndTexture.Load(int3(ipos, 0));
	norm = normalize(octToDir(asuint(nd.x)));
	zLinear = float2(nd.y, nd.z);
}

#endif