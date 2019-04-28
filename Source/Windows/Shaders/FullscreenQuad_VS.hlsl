
// ----------------------------------------------------------------------------------------------------------------------------

struct VertexInput
{
    uint Id : SV_VERTEXID;
};

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

    OUT.TexCoord = float2(uint2(id, id << 1) & 2);
    OUT.Position = float4(lerp(float2(-1, 1), float2(1, -1), OUT.TexCoord), 0, 1);

    return OUT;
}
