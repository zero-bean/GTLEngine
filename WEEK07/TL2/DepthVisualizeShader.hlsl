// DepthVisualizeShader.hlsl

Texture2D<float> DepthTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer CameraInfo : register(b0)
{
    float NearClip;
    float FarClip;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUTPUT mainVS(uint vid : SV_VertexID)
{
    VS_OUTPUT output;
    float2 pos[3] =
    {
        float2(-1.0f, 3.0f),
        float2(3.0f, -1.0f),
        float2(-1.0f, -1.0f)
    };
    
    // full screen triangle
    // ...should be CW (winding order)
    output.position.xy = pos[vid];
    output.position.z = 0.0f;
    output.position.w = 1.0f;
    
    // [-1, 1]: clip space → [0, 1]: texture space
    // DirectX texture coordinate: Y-axis flipped
    output.texcoord = 0.5f * (pos[vid] + 1.0f);
    output.texcoord.y = 1.0f - output.texcoord.y;
    
    return output;
}

float LinearizeDepth(float rawDepth)
{
    // The depth buffer stores z in a non-linear way due to perspective projection.
    // : The projection matrix (M_proj) maps view-space depth (Z_view) 
    //   into a non-linear NDC depth (Z_ndc) that is approximately proportional to 1/Z_view.
    // Linear depth represents the actual distance in the camera's space. (view)
    
    // Z_ndc = Z_clip / W_clip = F/(F-N) - (F*N)/(F-N) * Z_view → Z_view = ~
    float z = NearClip * FarClip / (FarClip - rawDepth * (FarClip - NearClip));
    
    // Normalize the linear depth to [0,1] by dividing by FarClip
    return z / FarClip;
}

float4 mainPS(VS_OUTPUT input) : SV_Target
{
    // Sampling result of the depth texture corresponding to the UV coordinate.
    float rawDepth = DepthTexture.Sample(PointSampler, input.texcoord).r;
    
    // If the sampled depth value is 1.0, it means the pixel is on the far plane
    // (no geometry rendered there, return white.)
    if (rawDepth >= 1.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Convert non-linear depth buffer value z(0~1) to linear depth in view space.
    float linearDepth = LinearizeDepth(rawDepth);
    
    // Output the linear depth as grayscale (brighter = farther)
    return float4(linearDepth, linearDepth, linearDepth, 1.0f);
}
