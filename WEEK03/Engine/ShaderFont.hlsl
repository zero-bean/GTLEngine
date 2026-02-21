cbuffer Constants : register(b0)
{
    row_major float4x4 MVP;
}

cbuffer FontConstants : register(b2)
{                  //  x,  y,  z,  w
    float2 cellSize;
    int atlasCols;
    float pad;
}

// t0 레지스터 바인딩: PS에서 사용할 2D Texture Resource
Texture2D Texture : register(t0);
// s0 레지스터 바인딩: 샘플러 객체
SamplerState Sampler : register(s0);

struct VSInput
{
    float3 position : POSITION; // Vertex position (x, y, z)
    float2 texCoord : TEXCOORD0; // Texture coordinates (u, v)
    float  charCode : TEXCOORD2;
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
    
    const int cx = input.charCode % atlasCols;
    const int cy = input.charCode / atlasCols;
    float2 baseUV = float2(cx, cy) * cellSize;
    float2 offsetUV = input.texCoord * cellSize;
   
    output.texCoord = baseUV + offsetUV;
    
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

    // 컬러의 알파가 0.1 이하면 픽셀 컷 아웃
    clip(inputColor.a - 0.1f);

    return inputColor;
}