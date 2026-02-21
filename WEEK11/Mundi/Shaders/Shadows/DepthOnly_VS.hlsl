// b0: ModelBuffer (VS) - ModelBufferType과 정확히 일치 (128 bytes)
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix; // 64 bytes
    row_major float4x4 WorldInverseTranspose; // 64 bytes - 올바른 노멀 변환을 위함
};

// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;   // 0행 광원의 월드 좌표 + 스포트라이트 반경
    row_major float4x4 InverseProjectionMatrix;
};

// b6: BonesBuffer (VS) - GPU 스키닝용 본 매트릭스 배열
#ifdef GPU_SKINNING
cbuffer BonesBuffer : register(b6)
{
    row_major float4x4 BoneMatrices[256];  // 최대 256개 본 지원
};
#endif

// --- 셰이더 입출력 구조체 ---
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT0;
    float4 Color : COLOR;
#ifdef GPU_SKINNING
    uint4 BoneIndices : BLENDINDICES;   // 각 정점에 영향을 주는 본 인덱스 (최대 4개)
    float4 BoneWeights : BLENDWEIGHT;   // 각 본의 가중치 (합=1.0)
#endif
};

// 출력은 오직 클립 공간 위치만 필요
struct VS_OUT
{
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD0;
};

VS_OUT mainVS(VS_INPUT Input)
{
    VS_OUT Output = (VS_OUT) 0;

#ifdef GPU_SKINNING
    // GPU 스키닝: 본 매트릭스를 사용하여 정점을 변환
    float3 skinnedPosition = float3(0.0f, 0.0f, 0.0f);

    // 최대 4개의 본에 대해 가중치 적용
    for (int i = 0; i < 4; i++)
    {
        uint boneIndex = Input.BoneIndices[i];
        float boneWeight = Input.BoneWeights[i];

        // 가중치가 0이면 스킵 (최적화)
        if (boneWeight > 0.0f)
        {
            // 본 매트릭스로 정점 위치 변환
            float4 localPos = mul(float4(Input.Position, 1.0f), BoneMatrices[boneIndex]);
            skinnedPosition += localPos.xyz * boneWeight;
        }
    }

    // 스키닝된 정점 데이터 사용
    float3 finalPosition = skinnedPosition;
#else
    // CPU 스키닝: 입력 정점을 그대로 사용
    float3 finalPosition = Input.Position;
#endif

    // 모델 좌표 -> 월드 좌표 -> 뷰 좌표 -> 클립 좌표
    float4 WorldPos = mul(float4(finalPosition, 1.0f), WorldMatrix);
    float4 ViewPos = mul(WorldPos, ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.WorldPosition = WorldPos.xyz;

    return Output;
}