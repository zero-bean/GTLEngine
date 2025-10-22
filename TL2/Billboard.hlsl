//UUID만 필요하지만 기존 버퍼와 함수 재사용하기 위해서 모델버퍼 받아옴
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    uint UUID;
    float3 Padding;
}
cbuffer CameraInfo : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
}
// C++에서 상수 버퍼를 통해 전달될 데이터
cbuffer BillboardBuffer : register(b2)
{
    float3 textWorldPos;
    float padding;
    row_major matrix viewInverse;
}

cbuffer ColorBufferType : register(b3)
{
    float4 SpriteColor;
}

// C++의 BillboardCharInfo와 레이아웃이 동일해야 하는 입력 구조체
struct VS_INPUT
{
    float3 centerPos : WORLDPOSITION;
    float2 size : SIZE;
    float4 uvRect : UVRECT;
    uint vertexId : SV_VertexID; // GPU가 자동으로 부여하는 고유 정점 ID
};

struct PS_INPUT
{
    float4 pos_screenspace : SV_POSITION;
    float2 tex : TEXCOORD0;
    uint UUID : UUID;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

Texture2D fontAtlas : register(t0);
SamplerState linearSampler : register(s0);


PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 카메라 회전 무시 (뷰 역행렬로 변환)
    float3 pos_aligned = mul(float4(input.centerPos, 0.0f), viewInverse).xyz;

    // 월드 좌표계에서 최종 위치 계산
    float3 finalPos_worldspace = textWorldPos + pos_aligned;

    // 뷰-프로젝션 변환
    output.pos_screenspace = mul(float4(finalPos_worldspace, 1.0f), mul(ViewMatrix, ProjectionMatrix)) * 0.1;

    // UV 계산: vertexId % 4로 코너 결정
    // 0=좌상단(0,0), 1=우상단(1,0), 2=좌하단(0,1), 3=우하단(1,1)
    uint corner = input.vertexId % 4;
    float u = input.uvRect.x + ((corner == 1 || corner == 3) ? input.uvRect.z : 0.0f);
    float v = input.uvRect.y + ((corner == 2 || corner == 3) ? input.uvRect.w : 0.0f);
    output.tex = float2(u, v);
    output.UUID = UUID;
    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;
    float4 color = fontAtlas.Sample(linearSampler, input.tex);

    clip(color.a - 0.5f); // alpha - 0.5f < 0 이면 해당픽셀 렌더링 중단

    if (SpriteColor.a > 0.0f)
    {
        color.rgb = color.rgb * SpriteColor.rgb;
        //color = color / (color + 1.0f);
        color = 1.0f - exp(-color * 1.25f);
        color = pow(color, 1.0f / 2.2f);
    }
    Result.Color = color;
    Result.UUID = input.UUID;
    return Result;
}