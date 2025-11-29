Texture2D g_SceneColorTex : register(t0);
Texture2D g_COCTex : register(t1);
Texture2D g_NearBlurTex : register(t2);
Texture2D g_FarBlurTex : register(t3);

SamplerState g_LinearClampSample : register(s0);
SamplerState g_PointClampSample : register(s1);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer PostProcessCB : register(b0)
{
    float Near;
    float Far;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
}

cbuffer DepthOfFieldCB : register(b2)
{
    
}

cbuffer ViewportConstants : register(b10)
{
    // x: Viewport TopLeftX, y: Viewport TopLeftY
    // z: Viewport Width,   w: Viewport Height
    float4 ViewportRect;
    
    // x: Screen Width      (전체 렌더 타겟 너비)
    // y: Screen Height     (전체 렌더 타겟 높이)
    // z: 1.0f / Screen W,  w: 1.0f / Screen H
    float4 ScreenSize;
}

float GetLinearDepth(float NDCDepth)
{
    float ViewZ = -Far * Near / (NDCDepth * (Far - Near) - Far);
    return saturate((ViewZ - Near) / (Far - Near));
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    uint TexWidth, TexHeight;
    g_SceneColorTex.GetDimensions(TexWidth, TexHeight);
    float2 uv = float2(input.position.x / TexWidth, input.position.y / TexHeight);
    
    float2 COC = g_COCTex.Sample(g_PointClampSample, uv).rg;

    float Origin = 1 - COC.r - COC.g;
    float3 SceneColor = g_SceneColorTex.Sample(g_LinearClampSample, uv);
    float3 NearColor = g_NearBlurTex.Sample(g_LinearClampSample, uv);
    float3 FarColor = g_FarBlurTex.Sample(g_LinearClampSample, uv);
    
    float3 FinalColor = SceneColor * Origin + NearColor * COC.r + FarColor * COC.g;
    return float4(FinalColor, 1);
}
