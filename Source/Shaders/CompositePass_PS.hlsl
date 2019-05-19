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
#include "../RealtimeEngine/RealtimeSceneShaderInclude.h"
#include "../RendererShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float2 TexCoord : TEXCOORD;
};

// ----------------------------------------------------------------------------------------------------------------------------

Texture2D                               AOTexture               : register(t0);
Texture2D                               CpuResultsTex           : register(t1);
Texture2D                               PositionsTexture        : register(t2);
Texture2D                               NormalsTexture          : register(t3);
Texture2D                               TexCoordsTexture        : register(t4);
Texture2D                               DiffuseTexture          : register(t5);
SamplerState                            LinearRepeatSampler     : register(s0);
ConstantBuffer<SceneConstantBuffer>     SceneCb                 : register(b0);

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    float4 texColor = DiffuseTexture.Sample(LinearRepeatSampler, IN.TexCoord) * SceneCb.TextureMultipliers[1];
    float4 ao       = AOTexture.Sample(LinearRepeatSampler, IN.TexCoord)      * SceneCb.TextureMultipliers[2];
    float4 cpuRT    = CpuResultsTex.Sample(LinearRepeatSampler, IN.TexCoord)  * SceneCb.TextureMultipliers[3];

    float4 finalCol = 
        ((texColor * ao)         * SceneCb.CompositeMultipliers[0]) + 
        ((texColor + ao + cpuRT) * SceneCb.CompositeMultipliers[1]);

    return finalCol;
}
