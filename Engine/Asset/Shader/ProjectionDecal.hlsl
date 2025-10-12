/* *
* @brief 오브젝트의 월드 좌표, 월드 노말, 클립 공간 좌표를 구하는데 사용되는 상수 버퍼입니다.
* @param WorldInverseTranspose: 오브젝트의 올바른 월드 노말을 구하는데 사용됩니다.
*                               스케일 및 회전으로 인한 노말 정보의 왜곡을 방지할 수 있습니다.
*                               [T * N = 0 => (M x T) * ( G x N ) = 0 => (M x T)^t x (G x N) = 0]
*/
cbuffer ModelConstantBuffer : register(b0)
{
	row_major float4x4 World; // 월드 행렬
	row_major float4x4 WorldInverseTranspose; // 월드 역전치 행렬
};

// C++의 FViewProjConstants와 일치
cbuffer ViewProjConstantBuffer : register(b1)
{
	row_major float4x4 View;
	row_major float4x4 Projection;
};

/* *
* @brief 데칼이 적용될 오브젝트의 범위를 계산하고 그리는데 사용되는 상수 버퍼입니다.
*/
cbuffer DecalConstantBuffer : register(b3)
{
	row_major float4x4 DecalInverseWorld; // 데칼의 월드 역행렬
	row_major float4x4 DecalWorld;
	float FadeProgress;				      // Fade progress (0-1)
	uint  FadeStyle;			          // 0:Standard, 1:WipeLtoR, 2:Dissolve, 3:Iris
	float2 padding;
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
	float3 WorldNormal : TEXCOORD1;
};

Texture2D DiffuseTexture : register(t0);
SamplerState DefaultSampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;

    // 1. 오브젝트의 로컬 좌표를 월드 좌표로 변환.
	output.WorldPos = mul(input.Position, World);
    // 2. 오브젝트의 로컬 노말을 월드 노말로 변환.
	output.WorldNormal = normalize(mul(input.Normal, (float3x3) WorldInverseTranspose));
    // 3. 오브젝트의 공간 정보를 최종 클립 좌표로 축소.
	output.Position = mul(mul(output.WorldPos, View), Projection);

	return output;
}

// UV 좌표로부터 무작위처럼 보이는 값을 생성하는 해시 함수
float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    // 1. 프로젝션 데칼의 투사 방향을 로컬 X축의 반대 방향으로 가정하고, 월드 공간으로 변환.
	float3 decalForward = normalize(mul(float4(1, 0, 0, 0), DecalWorld).xyz);

    // Case 1. 오브젝트와 데칼이 같은 방향을 바라보기 때문에 픽셀을 버립니다.
	if (dot(input.WorldNormal, decalForward) > 0.0f)
	{
        //discard;
	}

    // 2. 픽셀의 월드 좌표를 데칼의 로컬 공간으로 변환.
    // 'localPos'는 데칼의 단위 박스 내부에 있는 현재 픽셀의 3D 위치
	float3 localPos = mul(input.WorldPos, DecalInverseWorld).xyz;

    // Case 2. 프로젝션 데칼의 범위에 벗어나기 때문에 픽셀을 버립니다.
	if (abs(localPos.x) > 0.5f ||
        abs(localPos.y) > 0.5f ||
        abs(localPos.z) > 0.5f)
	{
		discard;
	}

    // 3. 픽셀의 3D 위치를 X축에서 바라보고 2D 평면(YZ)에 투사하여 UV를 생성.
	float2 decalUV = localPos.yz * float2(-1, -1) + 0.5f;

    // 4. 필터링을 통과한 픽셀에, 계산된 데칼 UV로 텍스처 색상을 출력.
	float4 decalColor = DiffuseTexture.Sample(DefaultSampler, decalUV);

    // Case 3. 데칼 텍스처의 알파(투명도) 값이 거의 없는 픽셀을 버립니다.
	if (decalColor.a < 0.01f)
	{
		discard;
	}

	// 5. FadeStyle에 따라 다른 효과를 적용합니다.
	float softness = 0.1f; // 경계선의 부드러움 정도

	// 진행도를 스케일링하여 0과 1 경계에서 softness를 위한 공간을 확보합니다.
	float scaledProgress = FadeProgress * (1.0f + softness);

	switch (FadeStyle)
	{
		case 0: // Standard Alpha Fade
			decalColor.a *= FadeProgress;
			break;
		case 1: // Wipe Left to Right
		{
			float threshold = decalUV.x;
			float t = saturate((scaledProgress - threshold) / softness);
			decalColor.a *= t * t * (3.0f - 2.0f * t);
			break;
		}
		case 2: // Procedural Random Dissolve
		{
			float threshold = hash(decalUV);
			float t = saturate((scaledProgress - threshold) / softness);
			decalColor.a *= t * t * (3.0f - 2.0f * t);
			break;
		}
		case 3: // Iris
		{
			float threshold = distance(decalUV, float2(0.5, 0.5)) * 1.414f;
			float t = saturate((scaledProgress - threshold) / softness);
			decalColor.a *= t * t * (3.0f - 2.0f * t);
			break;
		}
	}

    // Edge hardening to avoid fringe: suppress very low-alpha samples smoothly
    float edge = saturate((decalColor.a - 0.05f) / 0.05f); // 0 at 0.05, 1 at 0.10
    decalColor.a *= edge;
    clip(decalColor.a - 0.001f);

    // Premultiply to avoid white wash with standard alpha blending
    decalColor.rgb *= decalColor.a;
    return decalColor;
}
