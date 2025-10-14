cbuffer FullscreenDepthConstants : register(b0)
{
    row_major float4x4 InvViewProj;
    float4 CameraPosWSAndNear; // xyz: camera position, w: near clip
    float4 ViewportRect;        // xy: normalized top-left, zw: normalized size
    float4 FarAndPadding;       // x: far clip, yzw: padding
}

cbuffer HeightFogConstants : register(b1)
{
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;

    float FogMaxOpacity;
    float FogHeight;
    float _Pad0;
    float _Pad1;

    float4 FogInscatteringColor;
}

Texture2D DepthTexture : register(t0);
SamplerState SamplerPoint : register(s0);

struct VS_INPUT
{
    uint vertexID : SV_VertexID;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float2 GetFullUV(float2 localUV)
{
    return ViewportRect.xy + localUV * ViewportRect.zw;
}

float2 LinearEyeDistance(float2 localUV)
{
    float2 uv = GetFullUV(localUV);
    float depth01 = DepthTexture.SampleLevel(SamplerPoint, uv, 0).r;

    float2 ndcXY = uv * 2.0f - 1.0f;
    float zClip = depth01 * 2.0f - 1.0f;
    float4 clip = float4(ndcXY, zClip, 1.0f);

    float4 world = mul(clip, InvViewProj);
    world /= world.w;

    float2 result;
    result.x = distance(world.xyz, CameraPosWSAndNear.xyz);
    result.y = world.y;
    return result;
}

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.tex = uv * 0.5f;
    output.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float2 worldInfo = LinearEyeDistance(input.tex);
    float worldDepth = worldInfo.x;
    float worldY = worldInfo.y;

    float depth01 = DepthTexture.SampleLevel(SamplerPoint, GetFullUV(input.tex), 0).r;
    if (depth01 >= 0.9999f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float depth = max(0.0f, worldDepth - StartDistance);
    if (FogCutoffDistance > 0.0f && worldDepth > FogCutoffDistance)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float sigma = FogDensity * exp(-FogHeightFalloff * (worldY - FogHeight));
    sigma = max(sigma, 0.0f);

    float transmittance = exp(-sigma * depth);
    float alpha = saturate(min(1.0f - transmittance, FogMaxOpacity));

    float3 fogColor = FogInscatteringColor.rgb;
    return float4(fogColor, alpha);
}
