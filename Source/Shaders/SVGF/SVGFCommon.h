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

#define COMPARE_FUNC2(TYPE) \
bool2 equal(TYPE a, TYPE b)              { return bool2(a.x == b.x, a.y == b.y); } \
bool2 notEqual(TYPE a, TYPE b)           { return bool2(a.x != b.x, a.y != b.y); } \
bool2 lessThan(TYPE a, TYPE b)           { return bool2(a.x <  b.x, a.y <  b.y); } \
bool2 greaterThan(TYPE a, TYPE b)        { return bool2(a.x >  b.x, a.y >  b.y); } \
bool2 lessThanEqual(TYPE a, TYPE b)      { return bool2(a.x <= b.x, a.y <= b.y); } \
bool2 greaterThanEqual(TYPE a, TYPE b)   { return bool2(a.x >= b.x, a.y >= b.y); }

COMPARE_FUNC2(int2)
COMPARE_FUNC2(float2)

#undef COMPARE_FUNC2

#define COMPARE_FUNC3(TYPE) \
bool3 equal(TYPE a, TYPE b)              { return bool3(a.x == b.x, a.y == b.y, a.z == b.z); } \
bool3 notEqual(TYPE a, TYPE b)           { return bool3(a.x != b.x, a.y != b.y, a.z != b.z); } \
bool3 lessThan(TYPE a, TYPE b)           { return bool3(a.x <  b.x, a.y <  b.y, a.z <  b.z); } \
bool3 greaterThan(TYPE a, TYPE b)        { return bool3(a.x >  b.x, a.y >  b.y, a.z >  b.z); } \
bool3 lessThanEqual(TYPE a, TYPE b)      { return bool3(a.x <= b.x, a.y <= b.y, a.z <= b.z); } \
bool3 greaterThanEqual(TYPE a, TYPE b)   { return bool3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }

COMPARE_FUNC3(int3)
COMPARE_FUNC3(float3)

#undef COMPARE_FUNC3

#define COMPARE_FUNC4(TYPE) \
bool4 equal(TYPE a, TYPE b)              { return bool4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); } \
bool4 notEqual(TYPE a, TYPE b)           { return bool4(a.x != b.x, a.y != b.y, a.z != b.z, a.w != b.w); } \
bool4 lessThan(TYPE a, TYPE b)           { return bool4(a.x <  b.x, a.y <  b.y, a.z <  b.z, a.w <  b.w); } \
bool4 greaterThan(TYPE a, TYPE b)        { return bool4(a.x >  b.x, a.y >  b.y, a.z >  b.z, a.w >  b.w); } \
bool4 lessThanEqual(TYPE a, TYPE b)      { return bool4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); } \
bool4 greaterThanEqual(TYPE a, TYPE b)   { return bool4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }

COMPARE_FUNC4(int4)
COMPARE_FUNC4(float4)

#undef COMPARE_FUNC4

// ----------------------------------------------------------------------------------------------------------------------------

// Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space
// \param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
inline float luminance(float3 rgb)
{
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

// ----------------------------------------------------------------------------------------------------------------------------

int2 getTextureDims(Texture2D tex, uint mip)
{
    uint w, h;
    tex.GetDimensions(w, h);

    return int2(w, h);
}

// ----------------------------------------------------------------------------------------------------------------------------

int2 getTextureDims(RWTexture2D<float4> tex, uint mip)
{
    uint w, h;
    tex.GetDimensions(w, h);

    return int2(w, h);
}

// ----------------------------------------------------------------------------------------------------------------------------

uint packSnorm2x16(float2 val)
{
    uint l = asuint(f32tof16(val.x));
    uint h = asuint(f32tof16(val.y));

    return (h << 16) + l;
}

// ----------------------------------------------------------------------------------------------------------------------------

float2 unpackSnorm2x16(uint val)
{
    uint l = (val) & 0xffff;
    uint h = (val >> 16) & 0xffff;

    return float2(f16tof32(l), f16tof32(h));
}

// ----------------------------------------------------------------------------------------------------------------------------

float3 octToDir(uint octo)
{
    float2 e = float2(f16tof32(octo & 0xFFFF), f16tof32((octo >> 16) & 0xFFFF));
    float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy) * 2.0 - (float2)(1.0));
    return normalize(v);
}

// ----------------------------------------------------------------------------------------------------------------------------

uint dirToOct(float3 normal)
{
    float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
    float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p) * 2.0 - (float2)(1.0));
    return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

// ----------------------------------------------------------------------------------------------------------------------------

void fetchNormalAndLinearZ(in Texture2D ndTexture, in int2 ipos, out float3 norm, out float2 zLinear)
{
    float4 nd = ndTexture.Load(int3(ipos, 0));
    norm = normalize(octToDir(asuint(nd.x)));
    zLinear = float2(nd.y, nd.z);
}

// ----------------------------------------------------------------------------------------------------------------------------

float normalDistanceCos(float3 n1, float3 n2, float power)
{
    //return pow(max(0.0, dot(n1, n2)), 128.0);
    //return pow( saturate(dot(n1,n2)), power);
    return 1.0f;
}

// ----------------------------------------------------------------------------------------------------------------------------

float normalDistanceTan(float3 a, float3 b)
{
    const float d = max(1e-8, dot(a, b));

    return sqrt(max(0.0, 1.0 - d * d)) / d;
}

// ----------------------------------------------------------------------------------------------------------------------------

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
    const float wDirect    = exp(0.0 - max(wLdirect, 0.0) - max(wZ, 0.0)) * wNormal;
    const float wIndirect  = exp(0.0 - max(wLindirect, 0.0) - max(wZ, 0.0)) * wNormal;

    return float2(wDirect, wIndirect);
}

// ----------------------------------------------------------------------------------------------------------------------------

float computeWeightNoLuminance(float depthCenter, float depthP, float phiDepth, float3 normalCenter, float3 normalP)
{
    const float wNormal = normalDistanceCos(normalCenter, normalP, 128.0f);
    const float wZ      = abs(depthCenter - depthP) / phiDepth;

    return exp(-max(wZ, 0.0)) * wNormal;
}
