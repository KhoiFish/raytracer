
// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMatrices
{
    float4x4 ModelMatrix;
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ModelViewMatrix;
    float4x4 InverseTransposeModelViewMatrix;
    float4x4 ModelViewProjectionMatrix;
    float4x4 ShadowViewProj;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderMaterial
{
    float4 Emissive;
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float  SpecularPower;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct SpotLight
{
    float4 PositionWS;
    float4 PositionVS;
    float4 DirectionWS;
    float4 DirectionVS;
    float4 LookAtWS;
    float4 UpWS;
    float4 SmapWS;
    float4 Color;
    float  SpotAngle;
    float  ConstantAttenuation;
    float  LinearAttenuation;
    float  QuadraticAttenuation;
    int    ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct DirLight
{
    float4 DirectionWS;
    float4 DirectionVS;
    float4 Color;
    int    ShadowmapId;
};

// ----------------------------------------------------------------------------------------------------------------------------

struct GlobalData
{
    int   NumSpotLights;
    int   NumDirLights;
    float ShadowmapDepth;
};