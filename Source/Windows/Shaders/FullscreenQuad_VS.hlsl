
// ----------------------------------------------------------------------------------------------------------------------------

struct VertexInput
{
    uint Id : SV_VERTEXID;
};

// ----------------------------------------------------------------------------------------------------------------------------
struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;  // Note the position needs to be last, I don't know why??
};

// ----------------------------------------------------------------------------------------------------------------------------

VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;

    uint id = IN.Id;

    OUT.Position.x = (float)(id / 2) * 4.0f - 1.0f;
    OUT.Position.y = (float)(id % 2) * 4.0f - 1.0f;
    OUT.Position.z = 0.0f;
    OUT.Position.w = 1.0f;

    OUT.TexCoord.x = (float)(id / 2) * 2.0f;
    OUT.TexCoord.y = 1.0f - (float)(id % 2) * 2.0f;

    return OUT;
}
