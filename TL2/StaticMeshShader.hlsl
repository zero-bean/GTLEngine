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
    float3 CameraWorldPos; // 월드 기준 카메라 위치
    float _pad_cam; // 16바이트 정렬
}

cbuffer HighLightBuffer : register(b2)
{
    int Picked;
    float3 Color;
    int X;
    int Y;
    int Z;
    int GIzmo;
    int enable;
}

Texture2D g_DiffuseTexColor : register(t0);
Texture2D g_NormalTex : register(t1); // Normal map
SamplerState g_Sample : register(s0);

struct FMaterial
{
    float3 DiffuseColor; // Kd
    float OpticalDensity; // Ni
    float3 AmbientColor; // Ka
    float Transparency; // Tr/d
    float3 SpecularColor; // Ks
    float SpecularExponent; // Ns
    float3 EmissiveColor; // Ke
    uint IlluminationModel; // illum
    float3 TransmissionFilter; // Tf
    float dummy;
};

cbuffer ColorBuffer : register(b3)
{
    float4 LerpColor;
}

cbuffer PixelConstData : register(b4)
{
    FMaterial Material;
    bool HasMaterial;
    bool HasTexture;
    bool HasNormalTexture;
    float _pad_mat;
}

cbuffer PSScrollCB : register(b5)
{
    float2 UVScrollSpeed;
    float UVScrollTime;
    float _pad_scrollcb;
}

#define MAX_PointLight 100

// C++ 구조체와 동일한 레이아웃
struct FPointLightData
{
    float4 Position; // xyz=위치(월드), w=반경
    float4 Color; // rgb=색상, a=Intensity
    float FallOff; // 감쇠 지수
    float3 _pad; // 패딩
};

cbuffer PointLightBuffer : register(b9)
{
    int PointLightCount;
    float3 _pad;
    FPointLightData PointLights[MAX_PointLight];
}

struct LightAccum
{
    float3 diffuse;
    float3 specular;
};

// ------------------------------------------------------------------
// 안정화된 감쇠 + 방향(표면→광원) 버전의 simple 누적
// ------------------------------------------------------------------
float3 ComputePointLights(float3 worldPos)
{
    float3 total = 0;
    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        float3 Lvec = PointLights[i].Position.xyz - worldPos;
        float dist = length(Lvec);
        float range = max(PointLights[i].Position.w, 1e-3);
        float fall = max(PointLights[i].FallOff, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb * PointLights[i].Color.a;
        total += Li * atten;
    }
    return total;
}

// ------------------------------------------------------------------
// Lambert + Blinn-Phong (안정/일관성)
// ------------------------------------------------------------------
LightAccum ComputePointLights_LambertPhong(float3 worldPos, float3 worldNormal, float shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(CameraWorldPos - worldPos); // 픽셀 기준 뷰 벡터(월드)

    float exp = clamp(shininess, 1.0, 128.0); // 폭발 방지

    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        float3 Lvec = PointLights[i].Position.xyz - worldPos; // 표면→광원
        float dist = length(Lvec);
        float3 L = (dist > 1e-5) ? (Lvec / dist) : float3(0, 0, 1);

        float range = max(PointLights[i].Position.w, 1e-3);
        float fall = max(PointLights[i].FallOff, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb * PointLights[i].Color.a;

        // Diffuse
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * atten;

        // Specular (Blinn-Phong)
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float3 specular = Li * pow(NdotH, exp) * atten;

        acc.diffuse += diffuse;
        acc.specular += specular;
    }

    return acc;
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
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float3 worldTangent : TEXCOORD3;
    float3 worldBitangent : TEXCOORD4;
    float4 color : COLOR;
    uint UUID : UUID;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;
    
    float time = UVScrollTime;

    // 월드 변환 (row_major 기준: mul(v, M))
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
    o.worldPosition = worldPos.xyz;

    // 노멀, 탄젠트, 바이탄젠트를 월드 공간으로 변환
    o.worldNormal = normalize(mul(input.normal, (float3x3) NormalMatrix));
    o.worldTangent = normalize(mul(input.tangent, (float3x3) WorldMatrix));
    o.worldBitangent = normalize(mul(cross(o.worldNormal, o.worldTangent), (float3x3) WorldMatrix));

    // MVP
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    o.position = mul(float4(input.position, 1.0f), MVP);

    // Gizmo 색상 처리
    float4 c = input.color;
    if (GIzmo == 1)
    {
        if (Y == 1)
            c = float4(1.0, 1.0, 0.0, c.a);
        else
        {
            if (X == 1)
                c = float4(1.0, 0.0, 0.0, c.a);
            else if (X == 2)
                c = float4(0.0, 1.0, 0.0, c.a);
            else if (X == 3)
                c = float4(0.0, 0.0, 1.0, c.a);
        }
    }

    o.color = c;
    o.texCoord = input.texCoord;
    o.UUID = UUID;
    return o;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;

    float3 base = input.color.rgb;
    base = lerp(base, LerpColor.rgb, LerpColor.a) * (1.0f - (HasMaterial ? 1.0f : 0.0f));

    if (HasMaterial && HasTexture)
    {
        float2 uv = input.texCoord + UVScrollSpeed * UVScrollTime;
        base = g_DiffuseTexColor.Sample(g_Sample, uv).rgb;
    }
      
    if (Picked == 1)
    {
        // 노란색 하이라이트를 50% 블렌딩
        float3 highlightColor = float3(1.0, 1.0, 0.0); // 노란색
        base.rgb = lerp(base.rgb, highlightColor, 0.5);
    }

    // 조명 계산을 위한 노멀 벡터 준비
    float3 N = normalize(input.worldNormal);

    // 노말맵 텍스쳐가 존재한다면 
    if (HasNormalTexture)
    {
        // 1. 노멀맵 텍스쳐에서 RGB 값을 Normal 값으로 변환합니다.
        // RGB 값은 XYZ와 매핑되어 있으며 범위는 0~1로 저장되어 있고, 노말 값은 -1~1로 저장되어 있습니다.
        // Sample(): UV 좌표를 읽어와 샘플러스테이트의 규칙을 참고하여, 
        //           주변 텍셀의 색상을 조합해 해당 텍셀의 최종 색상값을 결정하는 역할을 가집니다.
        float3 tangentNormal = g_NormalTex.Sample(g_Sample, input.texCoord).rgb * 2.0 - 1.0;

        // 2. 보간된 벡터들로 TBN 행렬 재구성 및 직교화를 합니다.
        float3 T = normalize(input.worldTangent);
        float3 B = normalize(input.worldBitangent);
        float3 N_geom = normalize(input.worldNormal);
        
        // 정점 셰이더에서 픽셀 단위로 보간되어 넘어온 벡터들은 완벽하게 직교하지 않을 수 있습니다.
        // 따라서 그람-슈미트(Gram-Schmidt) 기법을 통해 TBN 좌표계를 다시 직교화하여 정렬합니다.
        T = normalize(T - dot(T, N_geom) * N_geom);
        B = cross(N_geom, T);

        // 3개의 기저 벡터를 기저 행렬로 변환합니다.
        float3x3 TBN = float3x3(T, B, N_geom);

        // 3. 탄젠트 공간 노멀을 월드 공간으로 변환합니다.
        N = normalize(mul(tangentNormal, TBN));
    }

    float shininess = (HasMaterial ? Material.SpecularExponent : 32.0); // 기본값 32
    LightAccum la = ComputePointLights_LambertPhong(input.worldPosition, N, shininess);
    
    // Ambient + Diffuse + Specular
    float3 ambient = 0.25 * base;
    if (HasMaterial)
        ambient += 0.25 * Material.AmbientColor;

    float3 diffuseLit = base * la.diffuse;
    float3 specularLit = la.specular;
    if (HasMaterial)
        specularLit *= saturate(Material.SpecularColor);

    float3 finalLit = ambient + diffuseLit + specularLit;
    finalLit = saturate(finalLit); // 과포화 방지
    
    if (GIzmo == 1)
    {
        finalLit = base;
    }
    
    Result.Color = float4(finalLit, 1.0);
    Result.UUID = input.UUID;
    return Result;
}
