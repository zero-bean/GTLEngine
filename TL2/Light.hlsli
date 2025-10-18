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





struct LightAccum
{
    float3 diffuse;
    float3 specular;
};


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

#endif