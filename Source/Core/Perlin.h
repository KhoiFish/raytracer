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

#pragma once

#include "Vec4.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Perlin
{
public:

    static inline float Noise(const Vec4& p)
    {
        float u = p.X() - floor(p.X());
        float v = p.Y() - floor(p.Y());
        float w = p.Z() - floor(p.Z());

        // Hermite cubic to round off interpolation
        u = u * u * (3 - 2 * u);
        v = v * v * (3 - 2 * v);
        w = w * w * (3 - 2 * w);

        int i = (int)floor(p.X());
        int j = (int)floor(p.Y());
        int k = (int)floor(p.Z());

        Vec4 c[2][2][2];
        for(int di = 0; di < 2; di++)
        {
            for (int dj = 0; dj < 2; dj++)
            {
                for (int dk = 0; dk < 2; dk++)
                {
                    c[di][dj][dk] =
                        RanVec[
                            PermX[(i + di) & 255] ^ 
                            PermY[(j + dj) & 255] ^
                            PermZ[(k + dk) & 255]];
                }
            }
        }

        return PerlinInterp(c, u, v, w);
    }

    static inline float Turb(const Vec4& p, int depth = 7)
    {
        float accum = 0;
        Vec4  tempP = p;
        float weight = 1.f;
        for (int i = 0; i < depth; i++)
        {
            accum += weight * Noise(tempP);
            weight *= 0.5f;
            tempP *= 2;
        }

        return fabs(accum);
    }

private:

    static inline Vec4* perlinGenerate()
    {
        Vec4* p = new Vec4[256];
        for (int i = 0; i < 256; i++)
        {
            p[i] = UnitVector(Vec4(
                -1 + 2 * RandomFloat(),
                -1 + 2 * RandomFloat(),
                -1 + 2 * RandomFloat()));
        }

        return p;
    }

    static inline void Permute(int* p, int n)
    {
        for (int i = n - 1; i > 0; i--)
        {
            int target = int(RandomFloat() * (i + 1));
            int tmp = p[i];

            p[i] = p[target];
            p[target] = tmp;
        }
    }

    static inline int* PerlinGeneratePerm()
    {
        int* p = new int[256];
        for (int i = 0; i < 256; i++)
        {
            p[i] = i;
        }
        Permute(p, 256);

        return p;
    }

private:

    static Vec4*  RanVec;
    static int*   PermX;
    static int*   PermY;
    static int*   PermZ;
};