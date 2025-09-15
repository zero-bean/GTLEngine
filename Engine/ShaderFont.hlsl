cbuffer Constants: register(b0)
{
    row_major float4x4 MVP;

}
// t0 레지스터 바인딩: PS에서 사용할 2D Texture Resource
Texture2D Texture : register(t0);
// s0 레지스터 바인딩: 샘플러 객체
SamplerState Sampler : register(s0);
struct VSInput
{
    float3 Position : POSITION; // Vertex Position (x, y, z)
    float2 TexCoord : TEXCOORD; // Texture coordinates (u, v)
};
struct VSOutput
{
    float4 Position : SV_POSITION; // Vertex Position (x, y, z, w)
    float2 TexCoord : TEXCOORD; // Texture coordinates (u, v)
};
VSOutput VS_Main(VSInput Input)
{
    VSOutput Output = (VSOutput) 0;
    // 원래
    Output.Position = mul(float4(Input.Position, 1.0f), MVP);
    Output.TexCoord = Input.TexCoord;
   
    return Output;
}

struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};
float4 PS_Main(PSInput Input) : SV_TARGET
{
    // Get pixel color from texture using UV 
	float4 TextureColor = Texture.Sample(Sampler, Input.TexCoord);
	float4 Output = TextureColor;
    
	return Output;
}