cbuffer Constants : register(b0)
{
    row_major float4x4 MVP;
}

// t0 레지스터 바인딩: PS에서 사용할 2D Texture Resource
Texture2D Texture : register(t0);
// s0 레지스터 바인딩: 샘플러 객체
SamplerState Sampler : register(s0);

struct VSInput
{
    float3 position : POSITION; // Vertex position (x, y, z)
    float2 texCoord : TEXCOORD0; // Texture coordinates (u, v)
};

struct VSOutput
{
    float4 position : SV_POSITION; // Vertex position (x, y, z, w)
    float2 texCoord : TEXCOORD; // Texture coordinates (u, v)
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.position = mul(float4(input.position, 1.0f), MVP);
    output.texCoord = input.texCoord;
    
    return output;
}

struct PSInput
{
    float4 position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 inputColor = Texture.Sample(Sampler, input.TexCoord);
    return inputColor;
}