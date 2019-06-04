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

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// ----------------------------------------------------------------------------------------------------------------------------

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// ----------------------------------------------------------------------------------------------------------------------------

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// ----------------------------------------------------------------------------------------------------------------------------

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float schlick(float cosine, float refIdx)
{
    float r0 = (1 - refIdx) / (1 + refIdx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow(1 - cosine, 5);
}

// ----------------------------------------------------------------------------------------------------------------------------

class ONB
{
    inline float3 U()               { return Axis[0]; }
    inline float3 V()               { return Axis[1]; }
    inline float3 W()               { return Axis[2]; }
    inline float3 Local(float3 a)   { return (a.x * U()) + (a.y * V()) + (a.z * W()); }

    void BuildFromW(float3 n)
    {
        Axis[2] = normalize(n);

        float3 a;
        if (abs(W().x) > 0.9f)
        {
            a = float3(0, 1, 0);
        }
        else
        {
            a = float3(1, 0, 0);
        }

        Axis[1] = normalize(cross(W(), a));
        Axis[0] = cross(W(), V());
    }
    
    float3 Axis[3];
};

// ----------------------------------------------------------------------------------------------------------------------------

inline uint3 getIndices(ByteAddressBuffer indexBuffer, uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int  address   = (baseIndex * 4);

    return indexBuffer.Load3(address);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline RealtimeSceneVertex getVertex(ByteAddressBuffer indexBuffer, ByteAddressBuffer vertexBuffer, uint triangleIndex, float3 barycentrics)
{
    RealtimeSceneVertex v;
    v.Position = float3(0, 0, 0);
    v.Normal   = float3(0, 0, 0);
    v.TexCoord = float2(0, 0);

    const uint3 indices = getIndices(indexBuffer, triangleIndex);
    const int   stride  = (3 * 4) + (3 * 4) + (2 * 4);    
    for (uint i = 0; i < 3; i++)
    {
        int address = indices[i] * stride;

        v.Position += asfloat(vertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.Normal += asfloat(vertexBuffer.Load3(address)) * barycentrics[i];
        address += (3 * 4);

        v.TexCoord += asfloat(vertexBuffer.Load2(address)) * barycentrics[i];
    }

    return v;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float4 temporalAccumulate(int accumCount, float4 prevColor, float4 currColor)
{
    return (accumCount * prevColor + currColor) / (accumCount + 1);
}
