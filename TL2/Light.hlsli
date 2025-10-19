#ifndef _LIGHT__HLSLI_
#define _LIGHT__HLSLI_

#define MAX_PointLight 100
#define MAX_SpotLight 100

//===========PointLight==============// 
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

//===========SpotLight==============//
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

//===========DirectionalLight==============//
// C++ 구조체와 동일한 레이아웃
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

//===========SH Ambient Light==============//
// Multi-Probe SH Ambient Light
struct FSHProbeData
{
    float4 Position;           // xyz=프로브 위치, w=영향 반경
    float4 SHCoefficients[9];  // 9개 SH 계수
    float Intensity;           // 강도
    float3 Padding;            // 16바이트 정렬
};

#define MAX_SH_PROBES 8

cbuffer MultiSHProbeBuffer : register(b12)
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

//===========DirectionalLight==============//
LightAccum ComputeDirectionalLights_BlinnPhong(float3 CameraWorldPos, float3 WorldPosition, float3 WorldNormal, float Shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 NormalVector = normalize(WorldNormal);
    // 카메라로 향하는 방향 벡터
    float3 ViewVector = normalize(CameraWorldPos - WorldPosition);

    float exp = clamp(Shininess, 1.0, 128.0);

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

// ------------------------------------------------------------------
// PointLights: Lambert + Blinn-Phong (안정/일관성)
// ------------------------------------------------------------------
LightAccum ComputePointLights_BlinnPhong(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorldPos - worldPos); // 픽셀 기준 뷰 벡터(월드)

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

// ------------------------------------------------------------------
// SpotLights: Only Diffuse, //TODO: Add Blinn Phong
// ------------------------------------------------------------------
LightAccum ComputeSpotLights_BlinnPhong(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorldPos - worldPos);
    float exp = clamp(shininess, 1.0, 128.0);

    [loop]
    for (int i = 0; i < SpotLightCount; i++)
    {
        FSpotLightData light = SpotLights[i];

        // Direction and distance
        float3 LvecToLight = light.Position.xyz - worldPos; // surface -> light
        float dist = length(LvecToLight);
        float3 L = normalize(LvecToLight);

        // Distance attenuation (point light 와 동일) 
        float range = max(light.Position.w, 1e-3);
        float fall = max(light.FallOff, 0.001);
        float t = saturate(dist / range);
        float attenDist = pow(saturate(1.0 - t), fall); 
        // float attenDist = pow(saturate(1 / (light.AttFactor.x + range * light.AttFactor.y + range * range * light.AttFactor.z)), fall);
         
        // 선형 보간 
        float3 lightDir = normalize(light.Direction.xyz);
        float3 lightToWorld = normalize(-L); // light -> surface
        float cosTheta = dot(lightDir, lightToWorld);
         
        float thetaInner = cos(radians(light.InnerConeAngle));
        float thetaOuter = cos(radians(light.OuterConeAngle));
        
        float att = saturate(pow((cosTheta - thetaOuter) / max((thetaInner - thetaOuter), 1e-3), light.InAndOutSmooth));
          
        // Diffuse
        float3 Li = light.Color.rgb * light.Color.a;
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;
         
        acc.diffuse += diffuse;
        //TODO
        //acc.specular += specular;
    } 
    return acc;
}

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

#endif