/* *
* @brief 오브젝트의 월드 좌표, 월드 노말, 클립 공간 좌표를 구하는데 사용되는 상수 버퍼입니다.
* @param WorldInverseTranspose: 오브젝트의 올바른 월드 노말을 구하는데 사용됩니다.
*                               스케일 및 회전으로 인한 노말 정보의 왜곡을 방지할 수 있습니다.
*                               [T * N = 0 => (M x T) * ( G x N ) = 0 => (M x T)^t x (G x N) = 0]
*/
cbuffer ModelConstants : register(b0) 
{
	row_major float4x4 World;
	row_major float4x4 WorldInverseTranspose;
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

cbuffer OccluderConstants : register(b5) // 장애물(Occluder) 정보
{
	row_major float4x4 OccluderWorld;
	row_major float4x4 OccluderInverseWorld;
	float3 OccluderScale;
	float padding;
};

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPos : TEXCOORD0;
	float3 WorldNormal : NORMAL;
};

// Vertex Shader는 FireBallShader와 거의 동일합니다.
PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output = (PS_INPUT) 0;
	
	// 1. 오브젝트의 로컬 좌표를 월드 좌표로 변환.
	Output.WorldPos = mul(Input.Position, World);
    // 2. 오브젝트의 로컬 노말을 월드 노말로 변환.
	Output.WorldNormal = normalize(mul(Input.Normal, (float3x3) WorldInverseTranspose));
    // 3. 오브젝트의 공간 정보를 최종 클립 좌표로 축소.
	Output.Position = mul(Output.WorldPos, ViewMatrix);
	Output.Position = mul(Output.Position, ProjectionMatrix);
	
	return Output;
}

// Pixel Shader가 핵심입니다.
float4 mainPS(PS_INPUT input) : SV_Target
{
    // 1. 광원과 픽셀을 장애물(Occluder)의 로컬 공간으로 변환합니다.
    // 이렇게 하면 복잡한 월드 공간 대신, 원점에 위치한 단순한 박스와의 교차 검사로 문제를 단순화할 수 있습니다.
	float3 occluderLocalPos = mul(input.WorldPos, OccluderInverseWorld).xyz;
	float3 lightLocalPos = mul(float4(FireBallWorldPosition, 1.0), OccluderInverseWorld).xyz;

    // 2. 장애물 로컬 공간에서 광원에서 픽셀로 향하는 광선(Ray)을 정의합니다.
	float3 rayDir = occluderLocalPos - lightLocalPos;
	float distanceToPixel = length(rayDir); // 광원과 픽셀 사이의 실제 거리
	rayDir = normalize(rayDir);

    // 3. 광선-상자 교차 검사 (Ray-AABB Intersection Test)를 수행합니다.
    // 장애물은 로컬 공간의 원점을 중심으로 각 축으로 -0.5 ~ +0.5 크기를 가집니다.
	float3 boxMin = -0.5f * OccluderScale;
	float3 boxMax = 0.5f * OccluderScale;

    // Slab Test 기법을 사용하여 교차점을 찾습니다.
	float3 t1 = (boxMin - lightLocalPos) / rayDir;
	float3 t2 = (boxMax - lightLocalPos) / rayDir;
	float3 tMinVec = min(t1, t2);
	float3 tMaxVec = max(t1, t2);

	float tNear = max(max(tMinVec.x, tMinVec.y), tMinVec.z);
	float tFar = min(min(tMaxVec.x, tMaxVec.y), tMaxVec.z);

    // 4. 그림자 판정
    //    조건 1: 광선이 상자와 교차해야 함 (tNear < tFar)
    //    조건 2: 상자가 광선 시작점보다 앞에 있어야 함 (tFar > 0)
    //    조건 3 (가장 중요): 교차점이 현재 픽셀보다 광원에 더 가까워야 함 (tNear < distanceToPixel)
	if (tNear < tFar && tFar > 0 && tNear < distanceToPixel)
	{
        // 이 픽셀은 장애물에 의해 가려졌으므로 그림자 영역입니다.
        // 빛 계산을 수행하지 않고 그냥 버립니다(discard).
		discard;
	}

    // 5. 그림자 영역이 아니라면, 기존과 동일한 광원 계산을 수행합니다.
	float Dist = distance(input.WorldPos.xyz, FireBallWorldPosition);
	float Attenuation = 1.0 - smoothstep(FireBallRadius * FireBallRadiusFallOff, FireBallRadius, Dist);
	float3 LightDir = normalize(FireBallWorldPosition - input.WorldPos.xyz);
    
	float diffuse = 1.0f;
	if (dot(input.WorldNormal, input.WorldNormal) > 0.0001f)
	{
		diffuse = saturate(dot(normalize(input.WorldNormal), LightDir));
	}

	float strength = saturate(FireBallIntensity * Attenuation * diffuse);
	float3 overlay = FireBallColor.rgb;
    
    // 최종 빛의 색상을 반환합니다.
	return float4(overlay, strength);
}
