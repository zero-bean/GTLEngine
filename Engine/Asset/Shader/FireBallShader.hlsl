/* *
* @brief 오브젝트의 월드 좌표, 월드 노말, 클립 공간 좌표를 구하는데 사용되는 상수 버퍼입니다.
* @param WorldInverseTranspose: 오브젝트의 올바른 월드 노말을 구하는데 사용됩니다.
*                               스케일 및 회전으로 인한 노말 정보의 왜곡을 방지할 수 있습니다.
*                               [T * N = 0 => (M x T) * ( G x N ) = 0 => (M x T)^t x (G x N) = 0]
*/
cbuffer ModelConstants : register(b0)
{
	row_major float4x4 World; // 월드 행렬
	row_major float4x4 WorldInverseTranspose; // 월드 역전치 행렬
};

/* *
* @brief 카메라의 View 및 Projection 행렬 정보를 담는 상수 버퍼입니다.
*/
cbuffer ViewProjConstants : register(b1)
{
	row_major matrix ViewMatrix;
	row_major matrix ProjectionMatrix;
};

/* *
* @brief FireBallComponent의 유사 광원 효과에 사용되는 상수 버퍼입니다.
* @param FireBallWorldPosition: 오브젝트와 광원 간의 길이 및 방향 벡터를 구하는데 사용합니다.
* @param FireBallColor: 오브젝트에 투영할 색상입니다.
* @param FireBallRadius: 광원의 최대 범위로써 거리에 따른 빛의 세기를 결정합니다.
* @param FireBallIntensity: 광원의 밝기입니다.
* @param FireBallRadiusFallOff: 빛의 감쇠가 시작되는 지점입니다.
*/
cbuffer FireBallConstants : register(b4)
{
	float3 FireBallWorldPosition;
	float FireBallRadius;
	float4 FireBallColor;
	float FireBallIntensity;
	float FireBallRadiusFallOff;
};

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 TexCoord : TEXCOORD;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPos : TEXCOORD0;
	float3 Normal : NORMAL;
};

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output = (PS_INPUT) 0;

	// 1. 오브젝트의 로컬 좌표를 월드 좌표로 변환.
	Output.WorldPos = mul(Input.Position, World);
    // 2. 오브젝트의 로컬 노말을 월드 노말로 변환.
	Output.Normal = normalize(mul(Input.Normal, (float3x3) WorldInverseTranspose));
    // 3. 오브젝트의 공간 정보를 최종 클립 좌표로 축소.
	Output.Position = mul(Output.WorldPos, ViewMatrix);
	Output.Position = mul(Output.Position, ProjectionMatrix);
	
	return Output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    // 0. 현재 픽셀과 파이어볼 중심 사이의 거리.
	float Dist = distance(input.WorldPos.xyz, FireBallWorldPosition);
    // 0. 빛의 감쇠(attenuation) 계산.
	float Attenuation = 1.0 - smoothstep(FireBallRadius * FireBallRadiusFallOff, FireBallRadius, Dist);
    // 0. 광원 방향 벡터 계산 (픽셀 -> 광원).
	float3 LightDir = normalize(FireBallWorldPosition - input.WorldPos.xyz);

    // 1-A. 해당 오브젝트의 법선 벡터 데이터가 존재하지 않는다면, 확산값을 1로 적용합니다.
	float diffuse = 1.0f;
    // 1-B. 법선 벡터가 존재한다면 확산광 계산을 계산합니다.
	if (dot(input.Normal, input.Normal) > 0.0001f)
	{
		// 광원과 정점이 바라보는 방향의 유사도에 따라 보정값이 증가.
		diffuse = saturate(dot(normalize(input.Normal), LightDir));
	}

    // 2. 최종적으로 더해질 빛의 기여도 계산.
	float3 LightContribution = FireBallColor.rgb * FireBallIntensity * Attenuation * diffuse;

    // 3. 최종적으로 더해질 빛의 색상만 반환합니다.
	return float4(LightContribution, 1.0f);
}
