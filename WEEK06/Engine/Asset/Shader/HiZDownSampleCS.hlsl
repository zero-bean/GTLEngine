cbuffer cb0 : register(b0)
{
	uint2 g_TextureSize; // Size of the current mip level (output)
	uint g_MipLevel; // Current mip level being generated
	uint g_Padding;
};

Texture2D<float> g_InputTexture : register(t0); // Previous mip level (or full texture chain)
RWTexture2D<float> g_OutputTexture : register(u0); // Current mip level

SamplerState Sampler_LinearClamp : register(s0); // Declare a sampler state

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 이전 밉 레벨의 크기 (float2)
	float2 prevMipSize = (float2) g_TextureSize * 2.0f;

    // 이전 밉 레벨의 픽셀 좌표 (2x2 블록의 Top-left 코너)
	float2 prevMipCoord = (float2) DTid.xy * 2.0f;

    // 2x2 블록의 4개 픽셀 중심 좌표를 정규화된 UV로 변환

    // Top-Left (TL): prevMipCoord + (0.5, 0.5)
	float2 uv0 = (prevMipCoord + float2(0.5f, 0.5f)) / prevMipSize;
    // Top-Right (TR): prevMipCoord + (1.5, 0.5)
	float2 uv1 = (prevMipCoord + float2(1.5f, 0.5f)) / prevMipSize;
    // Bottom-Left (BL): prevMipCoord + (0.5, 1.5)
	float2 uv2 = (prevMipCoord + float2(0.5f, 1.5f)) / prevMipSize;
    // Bottom-Right (BR): prevMipCoord + (1.5, 1.5)
	float2 uv3 = (prevMipCoord + float2(1.5f, 1.5f)) / prevMipSize;


    // Sample 4 texels from the previous mip level
    // g_MipLevel - 1은 g_InputTexture 내에서 이전 밉 레벨을 지정 (일반적인 밉맵 생성 가정)
	float depth0 = g_InputTexture.SampleLevel(Sampler_LinearClamp, uv0, g_MipLevel - 1).r;
	float depth1 = g_InputTexture.SampleLevel(Sampler_LinearClamp, uv1, g_MipLevel - 1).r;
	float depth2 = g_InputTexture.SampleLevel(Sampler_LinearClamp, uv2, g_MipLevel - 1).r;
	float depth3 = g_InputTexture.SampleLevel(Sampler_LinearClamp, uv3, g_MipLevel - 1).r;

	float maxDepth = max(max(depth0, depth1), max(depth2, depth3));

	g_OutputTexture[DTid.xy] = maxDepth;
}
