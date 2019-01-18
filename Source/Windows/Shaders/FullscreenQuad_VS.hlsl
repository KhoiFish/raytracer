struct Mat
{
    matrix ModelMatrix;
    matrix ModelViewMatrix;
    matrix InverseTransposeModelViewMatrix;
    matrix ModelViewProjectionMatrix;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    uint   Id       : SV_VERTEXID;
};

struct VertexShaderOutput
{
    float4 PositionVS : POSITION;
    float3 NormalVS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    uint id = IN.Id;

    OUT.PositionVS.x = (float)(id / 2) * 4.0f - 1.0f;
    OUT.PositionVS.y = (float)(id % 2) * 4.0f - 1.0f;
    OUT.PositionVS.z = 0.0f;
    OUT.PositionVS.w = 1.0f;

    OUT.TexCoord.x = (float)(id / 2) * 2.0f;
    OUT.TexCoord.y = 1.0f - (float)(id % 2) * 2.0f;

    OUT.Position = OUT.PositionVS;
    OUT.NormalVS = mul((float3x3)MatCB.InverseTransposeModelViewMatrix, IN.Normal);

    return OUT;
}