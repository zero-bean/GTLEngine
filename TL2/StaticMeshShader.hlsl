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
#define MAX_SpotLight 100

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

// C++ 구조체와 동일한 레이아웃
struct FSpotLightData
{
    float4 Position; // xyz=위치(월드), w=반경
    float4 Color; // rgb=색상, a=Intensity
    float4 Direction;
 
    float InnerConeAngle; // 감쇠 지수
    float OuterConeAngle; // 감쇠 지수
    float FallOff;
    float InAndOutSmooth;
    
    float3 AttFactor;
    float SpotPadding;
    };

cbuffer SpotLightBuffer : register(b13)
{
    int SpotLightCount; 
    float3 SpotBufferPadding;
    FSpotLightData SpotLights[MAX_SpotLight];
}

struct FDirectionalLightData
{
    float4 Color;
    float3 Direction;
    int bEnableSpecular;
};

cbuffer FDirectionalLightBufferType : register(b11)
{
    int DirectionalLightCount;
    float3 Pad;
    FDirectionalLightData DirectionalLights[MAX_PointLight];
}

// Spherical Harmonics L2 Ambient Light (b10)
cbuffer SHAmbientLightBuffer : register(b10)
{
    float4 SHCoefficients[9];  // 9 RGB coefficients for L2 SH
    float SHIntensity;         // Global intensity multiplier
    float3 _pad_sh;            // 16-byte alignment
}

// Multi-Probe SH Ambient Light (b11)
struct FSHProbeData
{
    float4 Position;           // xyz=프로브 위치, w=영향 반경
    float4 SHCoefficients[9];  // 9개 SH 계수
    float Intensity;           // 강도
    float3 Padding;            // 16바이트 정렬
};

#define MAX_SH_PROBES 8

cbuffer MultiSHProbeBuffer : register(b11)
{
    int ProbeCount;
    float3 _pad_multiprobe;
    FSHProbeData Probes[MAX_SH_PROBES];
}

struct LightAccum
{
    float3 diffuse;
    float3 specular;
};

// ------------------------------------------------------------------
// Spherical Harmonics Basis Functions (L2, 9 coefficients)
// ------------------------------------------------------------------
float SHBasis0(float3 n) { return 0.282095; }
float SHBasis1(float3 n) { return 0.488603 * n.y; }
float SHBasis2(float3 n) { return 0.488603 * n.z; }
float SHBasis3(float3 n) { return 0.488603 * n.x; }
float SHBasis4(float3 n) { return 1.092548 * n.x * n.y; }
float SHBasis5(float3 n) { return 1.092548 * n.y * n.z; }
float SHBasis6(float3 n) { return 0.315392 * (3.0 * n.z * n.z - 1.0); }
float SHBasis7(float3 n) { return 1.092548 * n.x * n.z; }
float SHBasis8(float3 n) { return 0.546274 * (n.x * n.x - n.y * n.y); }

// ------------------------------------------------------------------
// Evaluate SH Lighting for given normal
// Returns irradiance (diffuse ambient) from SH coefficients
// ------------------------------------------------------------------
float3 EvaluateSHLighting(float3 normal)
{
    float3 result = 0.0;

    // Normalize input normal
    float3 n = normalize(normal);

    // Evaluate each SH band
    result += SHCoefficients[0] * SHBasis0(n);
    result += SHCoefficients[1] * SHBasis1(n);
    result += SHCoefficients[2] * SHBasis2(n);
    result += SHCoefficients[3] * SHBasis3(n);
    result += SHCoefficients[4] * SHBasis4(n);
    result += SHCoefficients[5] * SHBasis5(n);
    result += SHCoefficients[6] * SHBasis6(n);
    result += SHCoefficients[7] * SHBasis7(n);
    result += SHCoefficients[8] * SHBasis8(n);

    // Apply global intensity
    result *= SHIntensity;

    // Ensure positive values (SH can produce negative values)
    result = max(result, 0.0);

    return result;
}

// ------------------------------------------------------------------
// Evaluate Multi-Probe SH Lighting
// Blends multiple probes based on distance
// ------------------------------------------------------------------
float3 EvaluateMultiProbeSHLighting(float3 worldPos, float3 normal)
{
    if (ProbeCount == 0)
        return float3(0, 0, 0);

    float3 n = normalize(normal);
    float3 totalLighting = 0.0;
    float totalWeight = 0.0;

    // Blend all probes based on distance
    [loop]
    for (int i = 0; i < ProbeCount; ++i)
    {
        float3 probePos = Probes[i].Position.xyz;
        float radius = Probes[i].Position.w;

        float dist = length(worldPos - probePos);

        // Distance-based weight (inverse distance falloff)
        float weight = saturate(1.0 - (dist / radius));
        weight = weight * weight; // Square for smoother falloff

        if (weight > 0.001)
        {
            // Evaluate SH for this probe
            float3 probeLighting = 0.0;
            probeLighting += Probes[i].SHCoefficients[0].rgb * SHBasis0(n);
            probeLighting += Probes[i].SHCoefficients[1].rgb * SHBasis1(n);
            probeLighting += Probes[i].SHCoefficients[2].rgb * SHBasis2(n);
            probeLighting += Probes[i].SHCoefficients[3].rgb * SHBasis3(n);
            probeLighting += Probes[i].SHCoefficients[4].rgb * SHBasis4(n);
            probeLighting += Probes[i].SHCoefficients[5].rgb * SHBasis5(n);
            probeLighting += Probes[i].SHCoefficients[6].rgb * SHBasis6(n);
            probeLighting += Probes[i].SHCoefficients[7].rgb * SHBasis7(n);
            probeLighting += Probes[i].SHCoefficients[8].rgb * SHBasis8(n);

            probeLighting *= Probes[i].Intensity;

            totalLighting += probeLighting * weight;
            totalWeight += weight;
        }
    }

    // Normalize by total weight
    if (totalWeight > 0.001)
    {
        totalLighting /= totalWeight;
    }

    return max(totalLighting, 0.0);
}

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

LightAccum ComputeDirectionalLights(float3 WorldPosition, float3 WorldNormal, float Shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 NormalVector = normalize(WorldNormal);
    // 카메라로 향하는 방향 벡터
    float3 ViewVector = normalize(CameraWorldPos - WorldPosition);

    float exp = clamp(Shininess, 1.0, 128.0);

    exp = 128.0;
    [loop]
    for (int i = 0; i < DirectionalLightCount; ++i)
    {
        float3 LightColor = DirectionalLights[i].Color.rgb * DirectionalLights[i].Color.a;
        
        // 광원에서 물체를 향하는 방향 벡터에 부호를 바꿔서 물체에서 광원을 향하도록 함
        float3 LightVector = normalize(-DirectionalLights[i].Direction);
        // 표면 -> 광원 벡터와 물체의 법선벡터 내적
        float DiffuseFactor = saturate(dot(LightVector, NormalVector));
        float3 Diffuse = LightColor * DiffuseFactor;
        acc.diffuse += Diffuse;

        if (DirectionalLights[i].bEnableSpecular == 1)
        {
            float3 HalfVector = normalize(LightVector + ViewVector);
            float SpecularFactor = saturate(dot(HalfVector, NormalVector));
            float3 Specular = LightColor * pow(SpecularFactor, exp);
            acc.specular += Specular;
        }
    }

    return acc;
}

 
LightAccum ComputeSpotLights(float3 worldPos, float3 worldNormal, float shininess)
{
    LightAccum acc = (LightAccum)0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(CameraWorldPos - worldPos);
    float  exp = clamp(shininess, 1.0, 128.0);

    [loop]
    for (int i = 0; i < SpotLightCount; i++)
    {
        FSpotLightData light = SpotLights[i];

        // Direction and distance
        float3 LvecToLight = light.Position.xyz - worldPos; // surface -> light
        float  dist        = length(LvecToLight);
        float3 L = normalize(LvecToLight);

        // Distance attenuation (point light 와 동일) 
        float range     = max(light.Position.w, 1e-3);
        float fall      = max(light.FallOff, 0.001);
        float t         = saturate(dist / range);
        float attenDist = pow(saturate(1.0 - t), fall);

        
       // float attenDist = pow(saturate(1 / (light.AttFactor.x + range * light.AttFactor.y + range * range * light.AttFactor.z)), fall);
        
        
        // 선형 보간 
        float3 lightDir  = normalize(light.Direction.xyz);
        float3 lightToWorld = normalize(-L); // light -> surface
        float cosTheta = dot(lightDir, lightToWorld);
         
        float thetaInner = cos(radians(light.InnerConeAngle));
        float thetaOuter = cos(radians(light.OuterConeAngle));
        
        float att = saturate(pow( (cosTheta - thetaOuter) / max((thetaInner - thetaOuter), 1e-3), light.InAndOutSmooth));
          
        // Diffuse
        float3 Li    = light.Color.rgb * light.Color.a;
        float  NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;
         
        acc.diffuse  += diffuse;
        //TODO
        //acc.specular += specular;
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
    LightAccum ls = ComputeSpotLights(input.worldPosition, N, shininess);
    LightAccum ld = ComputeDirectionalLights(input.worldPosition, N, shininess); 
    
    la.diffuse += ls.diffuse + ld.diffuse;
    la.specular += ls.specular + ld.specular;    
    
    // Ambient + Diffuse + Specular
    float3 ambient = 0.25 * base;

    // Multi-Probe SH-based Ambient Lighting
    float3 shAmbient = EvaluateMultiProbeSHLighting(input.worldPosition, N) * base;

    // Add material ambient if exists
    float3 ambient = shAmbient;
    //float3 ambient = base*0.25;
    if (HasMaterial)
        ambient += 0.25 * Material.AmbientColor;

    float3 diffuseLit = base * la.diffuse;
    float3 specularLit = la.specular;
    //if (HasMaterial)
    //    specularLit *= saturate(Material.SpecularColor);

    float3 finalLit = ambient + diffuseLit + specularLit;
    finalLit = saturate(finalLit); // 과포화 방지
    
    if (GIzmo == 1)
    {
        finalLit = base;
    }
    
    if (!HasTexture)
    {
        Result.Color = float4(EvaluateMultiProbeSHLighting(input.worldPosition, N), 1.0);
        Result.UUID = input.UUID;
        return Result;
    }
    Result.Color = float4(finalLit, 1.0);
    Result.UUID = input.UUID;
    return Result;
}
