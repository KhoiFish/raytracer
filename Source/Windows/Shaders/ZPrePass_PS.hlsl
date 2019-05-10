// ----------------------------------------------------------------------------------------------------------------------------

struct PixelShaderInput
{
    float4 DepthPosition : TEXCOORD0;
};

// ----------------------------------------------------------------------------------------------------------------------------

float4 main(PixelShaderInput IN) : SV_Target
{
    float depthValue = IN.DepthPosition.z / IN.DepthPosition.w;

    return float4(depthValue, depthValue, depthValue, depthValue);
}
