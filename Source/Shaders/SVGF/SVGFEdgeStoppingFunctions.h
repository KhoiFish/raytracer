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

float normalDistanceCos(float3 n1, float3 n2, float power)
{
	//return pow(max(0.0, dot(n1, n2)), 128.0);
	//return pow( saturate(dot(n1,n2)), power);
	return 1.0f;
}

float normalDistanceTan(float3 a, float3 b)
{
	const float d = max(1e-8, dot(a, b));
	return sqrt(max(0.0, 1.0 - d * d)) / d;
}

float2 computeWeight(
	float depthCenter, float depthP, float phiDepth,
	float3 normalCenter, float3 normalP, float normPower, 
	float luminanceDirectCenter, float luminanceDirectP, float phiDirect,
	float luminanceIndirectCenter, float luminanceIndirectP, float phiIndirect)
{
	const float wNormal    = normalDistanceCos(normalCenter, normalP, normPower);
	const float wZ         = (phiDepth == 0) ? 0.0f : abs(depthCenter - depthP) / phiDepth;
	const float wLdirect   = abs(luminanceDirectCenter - luminanceDirectP) / phiDirect;
	const float wLindirect = abs(luminanceIndirectCenter - luminanceIndirectP) / phiIndirect;

	const float wDirect   = exp(0.0 - max(wLdirect, 0.0)   - max(wZ, 0.0)) * wNormal;
	const float wIndirect = exp(0.0 - max(wLindirect, 0.0) - max(wZ, 0.0)) * wNormal;

	return float2(wDirect, wIndirect);
}

float computeWeightNoLuminance(float depthCenter, float depthP, float phiDepth, float3 normalCenter, float3 normalP)
{
	const float wNormal    = normalDistanceCos(normalCenter, normalP, 128.0f);
	const float wZ         = abs(depthCenter - depthP) / phiDepth;

	return exp(-max(wZ, 0.0)) * wNormal;
}