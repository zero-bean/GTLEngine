Texture2D<float> DepthTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer CameraInfo : register(b0)
{
    float NearClip;
    float FarClip;
};

float4 main(float4 position : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
    float rawDepth = DepthTexture.Sample(PointSampler, texcoord).r;
    
    float linearDepth = pow(rawDepth, 50.0f);
    
    return float4(linearDepth, linearDepth, linearDepth, 1.0f);
};