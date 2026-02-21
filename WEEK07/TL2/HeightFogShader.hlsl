cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    float3 CameraPosition;
    float Pad;
}
cbuffer InvWorldBuffer : register(b3)
{
    row_major float4x4 ViewProjMatrixInverse;
}

cbuffer FogConstant : register(b8)
{
    float4 FogInscatteringColor;

    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    float FogMaxOpacityDistance;
    float FogMaxOpacity;
    float FogActorHeight;
    float Padding;
}

Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
SamplerState Sampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : UV;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

PS_INPUT mainVS(uint Input : SV_VertexID)
{
    PS_INPUT Output;

   
    float2 UVMap[] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
    };

    Output.UV = UVMap[Input];
    Output.Position = float4(Output.UV.x * 2.0f - 1.0f, 1.0f - (Output.UV.y * 2.0f), 0.0f, 1.0f);
    
    return Output;
}

PS_OUTPUT mainPS(PS_INPUT Input)
{
    PS_OUTPUT Output;
    
    uint Width, Height;
    DepthTexture.GetDimensions(Width, Height);
    
    float2 UVPosition = Input.Position.xy / float2(Width, Height);
    float4 OriginalColor = ColorTexture.Sample(Sampler, UVPosition);
    float Depth = DepthTexture.Sample(Sampler, UVPosition);
    
    float4 NDCPosition = float4((Input.UV.x - 0.5f) * 2.0f, (0.5f - Input.UV.y) * 2.0f, Depth, 1.0f);
    
    float4 WorldPosition4 = mul(NDCPosition, ViewProjMatrixInverse);
    float3 WorldPosition = WorldPosition4 /= WorldPosition4.w;

    
    //카메라부터 포그 적용될 점까지 거리
    float DistanceToPoint = length(CameraPosition - WorldPosition);
    //컷오프 이후, StartDistance 이전 밀도 0
    if(DistanceToPoint > FogCutoffDistance || DistanceToPoint < StartDistance)
    {
        Output.Color = OriginalColor;
        return Output;
    }
    //StartDistance부터 FogMaxOpacityDistance까지 선형적으로 밀도 증가(비어 람베르트에 곱해줌)
    float DistanceFactor = saturate((DistanceToPoint - StartDistance) / (FogMaxOpacityDistance - StartDistance));
    
    //비어-람베르트 공식(실제 포그 물리량) 위로갈수록 밀도가 낮아지므로 Transparency로 표현함. Falloff값이 커지면 Transparency도 커짐.
    float FogTransparency = exp(-(FogHeightFalloff * (WorldPosition.z - FogActorHeight)));
    //투명도 DensityFactor로 조절하고 거리가 멀면 더 짙어짐
    FogTransparency = exp(-(FogTransparency * FogDensity * DistanceToPoint));
    
    //실제 포그 밀도에 DistanceFactor 곱해서 clamp
    float FogFactor = clamp((1.0f - FogTransparency) * DistanceFactor, 0.0f, FogMaxOpacity);
    Output.Color.xyz = FogFactor * FogInscatteringColor.xyz + (1.0f - FogFactor) * OriginalColor.xyz;
    Output.Color.w = 1.0f;
    //Output.Color = OriginalColor;
    return Output;
}