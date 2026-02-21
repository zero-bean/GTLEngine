Texture2D<float> g_DepthTexture : register(t0);
RWTexture2D<float> g_HiZTextureMip0 : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    g_HiZTextureMip0[DTid.xy] = g_DepthTexture[DTid.xy];
}
