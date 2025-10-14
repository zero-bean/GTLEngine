cbuffer constants : register(b0)
{
	row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;        // View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection;  // Projection Matrix Calculation of MVP Matrix
};

cbuffer PerFrame : register(b2)
{
	float4 totalColor;
};

struct VS_INPUT
{
    float4 position : POSITION;		// Input position from vertex buffer
    float4 color : COLOR;			// Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION;	// Transformed position to pass to the pixel shader
    float4 color : COLOR;			// Color to pass to the pixel shader
	float distWS : TEXCOORD2;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
	float4 tmp = input.position;
    tmp = mul(tmp, world);
	float3 Pos = tmp.xyz;

    tmp = mul(tmp, View);
    tmp = mul(tmp, Projection);

	float3x3 R = (float3x3)View;        // 상단-좌측 3x3
	float3  t  = View[3].xyz;           // 마지막 행 (row)
	float3  CameraPosWS = mul(-t, transpose(R)); // p = -t * R^T
	output.distWS = distance(Pos, CameraPosWS);

	output.position = tmp;
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float4 finalColor = lerp(input.color, totalColor, totalColor.a);

	float Depth = input.distWS /200.0f;
	finalColor = float4(Depth, Depth, Depth, 1.0f);

	return finalColor;
}
