
Texture2D FrameColor : register(t0);
SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);


cbuffer FGammaBufferType : register(b0)
{
    float Gamma;
    float3 padding;
};

struct PS_Input
{
    float4 posCS : SV_Position;
    float2 uv : TEXCOORD0;
};

PS_Input mainVS(uint Input : SV_VertexID)
{
    PS_Input o;
   
    float2 UVMap[] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
    };

    o.uv = UVMap[Input];
    o.posCS = float4(o.uv.x * 2.0f - 1.0f, 1.0f - (o.uv.y * 2.0f), 0.0f, 1.0f);
    return o;
}

float4 mainPS(PS_Input i) : SV_TARGET
{
    uint TexWidth, TexHeight, MipCount = 0;
    FrameColor.GetDimensions(0, TexWidth, TexHeight, MipCount);
    float2 TexSizeRCP = float2(1 / (float) TexWidth, 1 / (float) TexHeight);
    float2 uv = float2(i.posCS.x / TexWidth, i.posCS.y / TexHeight);
    float4 color = float4(FrameColor.Sample(LinearSampler, uv).rgb, 1);
    
    // 감마 보정: color^(1/gamma)
    color.rgb = pow(color.rgb, 1.0f / Gamma);
    
    return color;
}


