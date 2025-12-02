Texture2D g_SceneColorTex : register(t0);

SamplerState g_LinearClampSample : register(s0);
SamplerState g_PointClampSample : register(s1);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

static uint KernelCount = 13;
static float2 UVOffset[13] =
{
    float2(-2, -2), float2(0, -2), float2(2, -2),
    float2(-2, 0), float2(0, 0), float2(2, 0),
    float2(-2, 2), float2(0, 2), float2(2, 2),
    float2(-1, -1), float2(1, -1),
    float2(-1,1), float2(1,1)
};
static float2 UVWeight[13] =
{
    0.03125f, 0.0625f, 0.03125f,
    0.0625f, 0.125f, 0.0625f,
    0.03125f, 0.0625f, 0.03125f,
    0.125f, 0.125f, 0.125f, 0.125f
};

float4 mainPS(PS_INPUT input) : SV_Target
{
    uint TexWidth, TexHeight;
    g_SceneColorTex.GetDimensions(TexWidth, TexHeight);
    TexWidth *= 2;
    TexHeight *= 2;
    float2 uv = float2(input.position.x / TexWidth, input.position.y / TexHeight);
    float2 InvTexSize = float2(1.0f / TexWidth, 1.0f / TexHeight);
    
    float3 TotalColor = float3(0, 0, 0);
    for (int i = 0; i < KernelCount;i++)
    {
        float2 CurUV = uv + InvTexSize * UVOffset[i];
        float3 CurColor = g_SceneColorTex.Sample(g_LinearClampSample, CurUV).rgb;
        TotalColor += CurColor;
    }
    return float4(TotalColor, 1);
}
