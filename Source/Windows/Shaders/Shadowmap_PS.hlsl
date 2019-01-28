#include "ShaderStructs.hlsl"

// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 DepthPosition : TEXCOORD;
};

// ----------------------------------------------------------------------------------------------------------------------------

ConstantBuffer<GlobalData> GlobalDataCB : register(b1);

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    float depthValue = IN.DepthPosition.z / GlobalDataCB.ShadowmapDepth;

    return float4(depthValue, 0.f, 0.f, 1.f);
}
