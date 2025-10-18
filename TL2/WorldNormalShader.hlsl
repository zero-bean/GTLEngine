cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    uint UUID;
    float3 Padding;
    row_major float4x4 NormalMatrix;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    float3 CameraWorldPos; 
    float _pad_cam; 
}

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 worldNormal : TEXCOORD1;
    uint UUID : UUID;
};

// URenderer RenderTarget의 출력 개수와 일치해야하므로, 유의하시길 바랍니다.
struct PS_OUTPUT
{
    float4 Color : SV_Target0; // 노멀 컬러 출력
    uint UUID : SV_Target1;    // ID 버퍼 출력
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 로컬 법선 벡터와 월드 역전치 행렬곱을 하여, 월드 법선 벡터를 얻습니다.
    output.worldNormal = normalize(mul(input.normal, (float3x3) NormalMatrix));
    // 로컬 좌표 행렬을 월드 공간 기준으로 변환합니다.
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    output.position = mul(float4(input.position, 1.0f), MVP);
    
    output.UUID = UUID;

    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;

    // RGB 범위는 0~1 이므로, 노말 범위 -1 ~ 1을 변환하여 반환합니다.
    float3 NormalColor = (normalize(input.worldNormal) * 0.5f) + 0.5f;
    Result.Color = float4(NormalColor, 1.0f);
    
    Result.UUID = input.UUID; 

    return Result;
}