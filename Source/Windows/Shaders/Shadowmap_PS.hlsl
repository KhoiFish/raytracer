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
    float depthValue = IN.DepthPosition.z / IN.DepthPosition.w;

    return float4(depthValue, depthValue, depthValue, 1.f);
}
