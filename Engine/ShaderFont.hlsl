cbuffer Constants: register(b0)
{
    row_major float4x4 MVP;
    float4 MeshColor;
    float IsSelected;
    float3 Pad0;
    float3 BillboardCenterWorld;
    float SizeX;
    float SizeY;
    float3 Pad1;
}
cbuffer CBFrame : register(b1)
{
    float3 CameraRightWorld;
    float FPad0;
    float3 CameraUpWorld;
    float FPad1;
    float2 ViewportSize;
    // 화면축 정렬 모드 선택 : 0 = 월드 크기, 1 = 픽셀 고정
    int ScreenAlignMode;
    int FPad2;
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
    
    // ScreenAlignMode < 0  : 완전 일반(예전 폰트처럼 로컬 포지션 * MVP)
    // ScreenAlignMode == 0 : 카메라 Right/Up으로 월드크기 billboard
    // ScreenAlignMode == 1 : 픽셀 고정 billboard
    if (ScreenAlignMode < 0)
    {
        Output.Position = mul(float4(Input.Position, 1), MVP);
    }
    else if (ScreenAlignMode == 0)
    {
        float3 ws = BillboardCenterWorld
                  + CameraRightWorld * (Input.Position.x * SizeX)
                  + CameraUpWorld * (Input.Position.y * SizeY);
        Output.Position = mul(float4(ws, 1), MVP);
    }
    else
    {
        float4 Center = mul(float4(BillboardCenterWorld, 1), MVP);
        float2 Px = float2(Input.Position.x * SizeX, Input.Position.y * SizeY);
        float2 Ndc = float2(2 * Px.x / ViewportSize.x, -2 * Px.y / ViewportSize.y);
        Output.Position = float4(Center.xy + Ndc * Center.w, Center.z, Center.w);
    }
    // 원래
    // Output.Position = mul(float4(Input.Position, 1.0f), MVP);
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