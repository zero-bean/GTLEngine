struct FMaterial
{
    float3 DiffuseColor;
    float OpticalDensity;
    float3 AmbientColor;
    float Transparency;
    float3 SpecularColor;
    float SpecularExponent;
    float3 EmissiveColor;
    uint IlluminationModel;
    float3 TransmissionFilter;
    float dummy;
};

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
    float2 texCoord : TEXCOORD2;
    float3 worldTangent : TEXCOORD3;
    float3 worldBitangent : TEXCOORD4;
};

// URenderer RenderTarget의 출력 개수와 일치해야하므로, 유의하시길 바랍니다.
struct PS_OUTPUT
{
    float4 Color : SV_Target0;  // 노멀 컬러 출력
    uint UUID : SV_Target1;     // ID 버퍼 출력
};

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

cbuffer PixelConstData : register(b4)
{
    FMaterial Material;
    bool HasMaterial;
    bool HasTexture;
    bool bHasNormal;
    float _pad_mat;
}

Texture2D g_NormalTex : register(t1); 
SamplerState g_Sample : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 노멀, 탄젠트, 바이탄젠트를 월드 공간으로 변환
    // 로컬 법선 벡터와 월드 역전치 행렬곱을 하여, 월드 법선 벡터를 얻습니다.
    output.worldNormal = normalize(mul(input.normal, (float3x3) NormalMatrix));
    // Use NormalMatrix for tangent for consistent TBN under non-uniform scale
    output.worldTangent = normalize(mul(input.tangent, (float3x3) NormalMatrix));
    output.worldBitangent = normalize(cross(output.worldNormal, output.worldTangent));
    
    // 로컬 좌표 행렬을 월드 공간 기준으로 변환합니다.
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    output.position = mul(float4(input.position, 1.0f), MVP);

    // UUID
    output.UUID = UUID;

    output.texCoord = input.texCoord;

    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;

    // 1. 기본 지오메트리 노멀로 시작
    float3 N = normalize(input.worldNormal);

    if (bHasNormal)
    {
                // A-1. 노멀맵 텍스쳐에서 RGB 값을 Normal 값으로 변환합니다.
        // RGB 값은 XYZ와 매핑되어 있으며 범위는 0~1로 저장되어 있고, 노말 값은 -1~1로 저장되어 있습니다.
        // Sample(): UV 좌표를 읽어와 샘플러스테이트의 규칙을 참고하여, 
        //           주변 텍셀의 색상을 조합해 해당 텍셀의 최종 색상값을 결정하는 역할을 가집니다.
        float3 tangentNormal = g_NormalTex.Sample(g_Sample, input.texCoord).rgb * 2.0 - 1.0;

        // A-2. 보간된 벡터들로 TBN 행렬 재구성 및 직교화를 합니다.
        float3 T = normalize(input.worldTangent);
        float3 B = normalize(input.worldBitangent);
        float3 N_geom = normalize(input.worldNormal);
        
        // A- 3. 정점 셰이더에서 픽셀 단위로 보간되어 넘어온 벡터들은 완벽하게 직교하지 않을 수 있습니다.
        // 따라서 그람-슈미트(Gram-Schmidt) 기법을 통해 TBN 좌표계를 다시 직교화하여 정렬합니다.
        T = normalize(T - dot(T, N_geom) * N_geom);
        B = cross(N_geom, T);

        // A-4. 3개의 기저 벡터를 기저 행렬로 변환합니다.
        float3x3 TBN = float3x3(T, B, N_geom);

        // A-5. 탄젠트 공간 노멀을 월드 공간으로 변환합니다.
        N = normalize(mul(tangentNormal, TBN));
    }

    // 2. 최종 노멀(N)을 0~1 범위의 색상으로 변환
    float3 NormalColor = (N * 0.5f) + 0.5f;
    
    // 3. 최종 결과값을 반환합니다.
    Result.Color = float4(NormalColor, 1.0f); // SV_Target0
    Result.UUID = input.UUID;                 // SV_Target1

    return Result;
}
