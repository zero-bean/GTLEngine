// ShadowDepthParaboloid.hlsl
// Paraboloid projection depth shader for PointLight omnidirectional shadow maps
// Paraboloid mapping은 반구를 2D 텍스처에 투영하는 기법입니다

// b0: Object Transform
cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
};

// b1: View/Projection for light's perspective
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

// b9: Paraboloid Shadow 전용 상수 버퍼
cbuffer ParaboloidShadowBuffer : register(b9)
{
    float AttenuationRadius;  // 라이트 감쇠 반경
    float NearPlane;          // Near clipping plane
    uint bFrontHemisphere;    // 1 = 전면 반구 (+Z), 0 = 후면 반구 (-Z)
    float Padding;
};

struct VS_INPUT
{
    float3 Position : POSITION;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float Depth : TEXCOORD0;  // 선형 깊이 값
};

//-----------------------------------------------------------------------------
// Vertex Shader - Paraboloid Projection
//-----------------------------------------------------------------------------
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 1. World Space로 변환
    float4 worldPos = mul(float4(input.Position, 1.0f), WorldMatrix);

    // 2. View Space로 변환 (라이트 중심 기준)
    float4 viewPos = mul(worldPos, ViewMatrix);

    // 3. Paraboloid Projection 적용
    // View Space에서 Z축이 전면(+) 또는 후면(-) 방향
    float3 viewDir = normalize(viewPos.xyz);

    // 전면/후면 반구 선택에 따라 Z 부호 조정
    if (bFrontHemisphere == 0)
    {
        // 후면 반구: Z를 반전시켜 -Z 방향이 forward가 되도록
        viewDir.z = -viewDir.z;
    }

    // Paraboloid 투영 공식
    // x' = x / (1 + z)
    // y' = y / (1 + z)
    float denominator = 1.0f + viewDir.z;

    // Z가 음수면 해당 반구 범위를 벗어난 것 (clipping)
    if (denominator <= 0.0f)
    {
        // 범위를 벗어난 정점은 clip space 밖으로 이동
        output.Position = float4(0, 0, 0, 0);  // w=0이면 clip됨
        output.Depth = 0;
        return output;
    }

    // Paraboloid UV 좌표 계산 (-1 ~ 1 범위)
    float2 paraboloidUV;
    paraboloidUV.x = viewDir.x / denominator;
    paraboloidUV.y = viewDir.y / denominator;

    // 4. Clip Space로 변환 (NDC 범위: -1 ~ 1)
    output.Position.xy = paraboloidUV;
    output.Position.w = 1.0f;

    // 5. 깊이 계산 (선형 깊이, 0 ~ 1 범위)
    float distance = length(viewPos.xyz);
    output.Position.z = saturate((distance - NearPlane) / (AttenuationRadius - NearPlane));
    output.Depth = output.Position.z;

    return output;
}

//-----------------------------------------------------------------------------
// Pixel Shader
//-----------------------------------------------------------------------------
void mainPS(PS_INPUT input)
{
    // Paraboloid 셰이더에서는 깊이만 출력
    // SV_POSITION.z가 자동으로 depth buffer에 기록됨

    // 필요시 여기서 alpha testing이나 추가 처리 가능
}
