// Minimal FXAA 3.11-inspired implementation for DX11 (ps_5_0)

Texture2D SceneTex : register(t0);
SamplerState LinearClamp : register(s0);

cbuffer FXAAParams : register(b0)
{
    // xy: normalized offset (top-left), zw: normalized scale (width,height) into SceneTex
    float4 ViewportRect;
}

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

// Fullscreen triangle without vertex buffers using SV_VertexID
VSOut VS(uint vid : SV_VertexID)
{
    VSOut o;
    float2 pos = float2((vid == 2) ? 3.0 : -1.0, (vid == 1) ? 3.0 : -1.0);
    o.pos = float4(pos, 0.0, 1.0);
    o.uv = float2((pos.x + 1.0) * 0.5, 1.0 - (pos.y + 1.0) * 0.5);
    return o;
}

// Luma helper
float Luma(float3 rgb)
{
    // Rec. 601
    return dot(rgb, float3(0.299, 0.587, 0.114));
}

float4 PS(VSOut i) : SV_Target
{
    // Map [0,1] UV to the current viewport sub-rect
    float2 uv = ViewportRect.xy + i.uv * ViewportRect.zw;
    uint w, h;
    SceneTex.GetDimensions(w, h);
    float2 rcp = 1.0 / float2(w, h);

    // Sample neighborhood
    float3 rgbM = SceneTex.Sample(LinearClamp, uv).rgb;
    float lumaM = Luma(rgbM);
    float lumaN = Luma(SceneTex.Sample(LinearClamp, uv + float2(0, -rcp.y)).rgb);
    float lumaS = Luma(SceneTex.Sample(LinearClamp, uv + float2(0,  rcp.y)).rgb);
    float lumaW = Luma(SceneTex.Sample(LinearClamp, uv + float2(-rcp.x, 0)).rgb);
    float lumaE = Luma(SceneTex.Sample(LinearClamp, uv + float2( rcp.x, 0)).rgb);

    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float lumaRange = lumaMax - lumaMin;

    // Early exit if flat
    if (lumaRange < max(0.0312, lumaMax * 0.125))
    {
        return float4(rgbM, 1.0);
    }

    // Edge direction
    float2 dir;
    dir.x = -((lumaW + lumaE) - 2.0 * lumaM);
    dir.y = -((lumaN + lumaS) - 2.0 * lumaM);

    float dirReduce = max((lumaN + lumaS + lumaW + lumaE) * (0.25 * 1/8), 0.0004);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = saturate(dir * rcpDirMin) * rcp;
	// dir *= rcpDirMin;                 // scale by inverse smallest axis + reduce
	// float len = length(dir);
	// if (len > 0.0)
	// 	dir *= (min(8.0, len) / len); // cap search to ~8 texels total span
	//
	// dir *= rcp;

    // Two taps along the edge
    float3 rgbA = 0.5 * (
        SceneTex.Sample(LinearClamp, uv + dir * (1.0/3.0 - 0.5)).rgb +
        SceneTex.Sample(LinearClamp, uv + dir * (2.0/3.0 - 0.5)).rgb);

    float3 rgbB = rgbA * 0.5 + 0.25 * (
        SceneTex.Sample(LinearClamp, uv + dir * -0.5).rgb +
        SceneTex.Sample(LinearClamp, uv + dir * 0.5).rgb);

    float lumaB = Luma(rgbB);
    // Choose between A and B to avoid over-blurring
    float3 result = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
    return float4(result, 1.0);
}
