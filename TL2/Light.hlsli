#ifndef _LIGHT__HLSLI_
#define _LIGHT__HLSLI_

#define MAX_PointLight 100
#define MAX_SpotLight 100

static const float PI = 3.14159265359;

//===========PointLight==============// 
// C++ 구조체와 동일한 레이아웃
struct FPointLightData
{
    float4 Position; // xyz=위치(월드), w=반경
    float4 Color; // rgb=색상, a=falloff
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
    float4 Color; // rgb=색상, a=falloff
    float4 Direction;
 
    float InnerConeAngle; // 감쇠 지수
    float OuterConeAngle; // 감쇠 지수
    float InAndOutSmooth;
    float _pad;
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
    float4 Position;           // xyz=프로브 위치, w=BoxExtent.Z
    float4 SHCoefficients[9];  // 9개 SH 계수
    float Intensity;           // 강도
    float Falloff;             // 감쇠 지수
    float2 BoxExtent;          // xy=BoxExtent.X, BoxExtent.Y (Z는 Position.w에 저장)
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
//-----------------------Blinn-Phong Shading-----------------------//
// ------------------------------------------------------------------
// Directional Light : Blinn-Phong
// ------------------------------------------------------------------
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
        float3 LightColor = DirectionalLights[i].Color.rgb;
        
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
        float fall = max(PointLights[i].Color.w, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb;

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
// SpotLights : Blinn-Phong
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
        float fall = max(light.Color.w, 0.001);
        float t = saturate(dist / range);
        float attenDist = pow(saturate(1.0 - t), fall); 
         
        // 선형 보간 
        float3 lightDir = normalize(light.Direction.xyz);
        float3 lightToWorld = normalize(-L); // light -> surface
        float cosTheta = dot(lightDir, lightToWorld);
         
        float thetaInner = cos(radians(light.InnerConeAngle));
        float thetaOuter = cos(radians(light.OuterConeAngle));
        
        float att = saturate(pow((cosTheta - thetaOuter) / max((thetaInner - thetaOuter), 1e-3), light.InAndOutSmooth));
          
        // Diffuse
        float3 Li = light.Color.rgb;
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;
         
        acc.diffuse += diffuse;
        // Specular
        float3 HalfVector = normalize(L + V);
        float SpecularFactor = saturate(dot(HalfVector, N));
        float Specular = Li * pow(SpecularFactor, exp) * attenDist * att;
        acc.specular += Specular;

    } 
    return acc;
}

//--------------------------Phong Shading--------------------------//
// ------------------------------------------------------------------
// Directional Light : Phong
// ------------------------------------------------------------------
LightAccum ComputeDirectionalLights_Phong(float3 CameraWorldPos, float3 WorldPosition, float3 WorldNormal, float Shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 NormalVector = normalize(WorldNormal);
    // 카메라로 향하는 방향 벡터
    float3 ViewVector = normalize(CameraWorldPos - WorldPosition);

    float exp = clamp(Shininess, 1.0, 128.0);

    [loop]
    for (int i = 0; i < DirectionalLightCount; ++i)
    {
        float3 LightColor = DirectionalLights[i].Color.rgb;

        // Diffuse
        // 광원에서 물체를 향하는 방향 벡터에 부호를 바꿔서 물체에서 광원을 향하도록 함
        float3 LightVector = normalize(-DirectionalLights[i].Direction);
        // 표면 -> 광원 벡터와 물체의 법선벡터 내적
        float DiffuseFactor = saturate(dot(LightVector, NormalVector));
        float3 Diffuse = LightColor * DiffuseFactor;
        acc.diffuse += Diffuse;

        // Specular
        if (DirectionalLights[i].bEnableSpecular == 1)
        {
            float3 ReflectVector = reflect(DirectionalLights[i].Direction, NormalVector);
            float SpecularFactor = saturate(dot(ReflectVector, ViewVector));
            float3 Specular = LightColor * pow(SpecularFactor, exp);
            acc.specular += Specular;
        }
    }
    return acc;
}

// ------------------------------------------------------------------
// PointLights : Lambert + Phong
// ------------------------------------------------------------------
LightAccum ComputePointLights_Phong(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess)
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
        float fall = max(PointLights[i].Color.w, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb;

        // Diffuse
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * atten;

        // Specular (Phong)
        // Vector L은 표면에서 광원을 향하는 정규화된 벡터, 부호를 바꾸면 입사광
        float3 ReflectVector = reflect(-L, N);
        float SpecularFactor = saturate(dot(ReflectVector, V));
        float3 Specular = Li * pow(SpecularFactor, exp) * atten;

        acc.diffuse += diffuse;
        acc.specular += Specular;
    }

    return acc;
}

// ------------------------------------------------------------------
// SpotLights : Phong
// ------------------------------------------------------------------
LightAccum ComputeSpotLights_Phong(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess)
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
        float fall = max(light.Color.w, 0.001);
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
        float3 Li = light.Color.rgb;
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;
         
        acc.diffuse += diffuse;
        // Specular
        float3 ReflectVector = reflect(-L, N);
        float SpecularFactor = saturate(dot(ReflectVector, V));
        float Specular = Li * pow(SpecularFactor, exp) * attenDist * att;
        acc.specular += Specular;

    } 
    return acc;
}

//--------------------------Lambert Shading--------------------------//
// ------------------------------------------------------------------
// Directional Light : Lambert
// ------------------------------------------------------------------
LightAccum ComputeDirectionalLights_Lambert(float3 WorldNormal)
{
    LightAccum acc = (LightAccum) 0;

    float3 NormalVector = normalize(WorldNormal);
    
    [loop]
    for (int i = 0; i < DirectionalLightCount; ++i)
    {
        float3 LightColor = DirectionalLights[i].Color.rgb;

        // Diffuse
        // 광원에서 물체를 향하는 방향 벡터에 부호를 바꿔서 물체에서 광원을 향하도록 함
        float3 LightVector = normalize(-DirectionalLights[i].Direction);
        // 표면 -> 광원 벡터와 물체의 법선벡터 내적
        float DiffuseFactor = saturate(dot(LightVector, NormalVector));
        float3 Diffuse = LightColor * DiffuseFactor;
        acc.diffuse += Diffuse;
    }
    return acc;
}

// ------------------------------------------------------------------
// PointLights: Lambert
// ------------------------------------------------------------------
LightAccum ComputePointLights_Lambert(float3 worldPos, float3 worldNormal)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);

    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        float3 Lvec = PointLights[i].Position.xyz - worldPos; // 표면→광원
        float dist = length(Lvec);
        float3 L = (dist > 1e-5) ? (Lvec / dist) : float3(0, 0, 1);

        float range = max(PointLights[i].Position.w, 1e-3);
        float fall = max(PointLights[i].Color.w, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb;

        // Diffuse
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * atten;

        acc.diffuse += diffuse;
    }

    return acc;
}

// ------------------------------------------------------------------
// SpotLights
// ------------------------------------------------------------------
LightAccum ComputeSpotLights_Lambert(float3 worldPos, float3 worldNormal)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);

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
        float fall = max(light.Color.w, 0.001);
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
        float3 Li = light.Color.rgb;
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;
         
        acc.diffuse += diffuse;
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
    n = -n;
    float3 totalLighting = 0.0;
    float totalWeight = 0.0;

    // Blend all probes based on box distance
    [loop]
    for (int i = 0; i < ProbeCount; ++i)
    {
        float3 probePos = Probes[i].Position.xyz;
        float3 boxExtent = float3(Probes[i].BoxExtent.xy, Probes[i].Position.w);

        // Calculate distance from box center on each axis
        float3 offset = abs(worldPos - probePos);

        // Check if inside box (all axes within extent)
        bool3 insideBox = offset <= boxExtent;

        if (insideBox.x && insideBox.y && insideBox.z)
        {
            // Inside box: full influence with optional falloff from edges
            float3 normalizedDist = offset / max(boxExtent, 0.001);
            float distFromEdge = 1.0 - max(max(normalizedDist.x, normalizedDist.y), normalizedDist.z);
            float weight = pow(distFromEdge, Probes[i].Falloff);

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
    }

    // Normalize by total weight (preserve brightness when multiple probes overlap)
    // Using max(totalWeight, 1.0) prevents darkening when probes overlap
    if (totalWeight > 0.001)
    {
        totalLighting /= max(totalWeight, 1.0);
    }

    // SH lighting can be negative (represents directionality)
    // Clamping should be done after combining with albedo, not here
    return totalLighting;
}

// ------------------------------------------------------------------
// Tile-based Light Culling Support
// ------------------------------------------------------------------

#define TILE_SIZE 32

StructuredBuffer<uint> g_LightIndexList : register(t10);
StructuredBuffer<uint2> g_TileOffsetCount : register(t11);

cbuffer TileCullingInfoCB : register(b6)
{
    uint NumTilesX;
    uint DebugVisualizeTiles; // 0 = off, 1 = on
    float2 ViewportOffset;
}

// Tile-based PointLights: Blinn-Phong (optimized version)
LightAccum ComputePointLights_BlinnPhong_Tiled(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess, float4 screenPos)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorldPos - worldPos);
    float exp = clamp(shininess, 1.0, 128.0);

    // Calculate tile index from screen position
    uint2 tileID = uint2(screenPos.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * NumTilesX + tileID.x;

    // Get light list for this tile
    uint2 offsetCount = g_TileOffsetCount[tileIndex];
    uint lightListOffset = offsetCount.x;
    uint lightCount = offsetCount.y;

    // Iterate only over lights affecting this tile
    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIndex = g_LightIndexList[lightListOffset + i];

        // Check if this is a point light (spot lights come after)
        if (lightIndex >= (uint)PointLightCount)
            continue;

        float3 Lvec = PointLights[lightIndex].Position.xyz - worldPos;
        float dist = length(Lvec);
        float3 L = (dist > 1e-5) ? (Lvec / dist) : float3(0, 0, 1);

        float range = max(PointLights[lightIndex].Position.w, 1e-3);
        float fall = max(PointLights[lightIndex].Color.w, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[lightIndex].Color.rgb;

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

// Tile-based SpotLights: Blinn-Phong (optimized version)
LightAccum ComputeSpotLights_BlinnPhong_Tiled(float3 cameraWorldPos, float3 worldPos, float3 worldNormal, float shininess, float4 screenPos)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(cameraWorldPos - worldPos);
    float exp = clamp(shininess, 1.0, 128.0);

    // Calculate tile index from screen position
    uint2 tileID = uint2(screenPos.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * NumTilesX + tileID.x;

    // Get light list for this tile
    uint2 offsetCount = g_TileOffsetCount[tileIndex];
    uint lightListOffset = offsetCount.x;
    uint lightCount = offsetCount.y;

    // Iterate only over lights affecting this tile
    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIndex = g_LightIndexList[lightListOffset + i];

        // Check if this is a spot light
        if (lightIndex < (uint)PointLightCount)
            continue;

        uint spotIndex = lightIndex - (uint)PointLightCount;
        if (spotIndex >= (uint)SpotLightCount)
            continue;

        FSpotLightData light = SpotLights[spotIndex];

        // Direction and distance
        float3 LvecToLight = light.Position.xyz - worldPos;
        float dist = length(LvecToLight);
        float3 L = normalize(LvecToLight);

        // Distance attenuation
        float range = max(light.Position.w, 1e-3);
        float fall = max(light.Color.a, 0.001);
        float t = saturate(dist / range);
        float attenDist = pow(saturate(1.0 - t), fall);

        // Cone attenuation
        float3 lightDir = normalize(light.Direction.xyz);
        float3 lightToWorld = normalize(-L);
        float cosTheta = dot(lightDir, lightToWorld);

        float thetaInner = cos(radians(light.InnerConeAngle));
        float thetaOuter = cos(radians(light.OuterConeAngle));

        float att = saturate(pow((cosTheta - thetaOuter) / max((thetaInner - thetaOuter), 1e-3), light.InAndOutSmooth));

        // Diffuse
        float3 Li = light.Color.rgb * light.Color.a;
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * attenDist * att;

        // Specular (Blinn-Phong)
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float3 specular = Li * pow(NdotH, exp) * attenDist * att;

        acc.diffuse += diffuse;
        acc.specular += specular;
    }

    return acc;
}

// Helper function to get light count for debug visualization
uint GetTileLightCount(float4 screenPos)
{
    uint2 tileID = uint2(screenPos.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * NumTilesX + tileID.x;
    uint2 offsetCount = g_TileOffsetCount[tileIndex];
    return offsetCount.y; // Return light count
}

// Debug visualization: Convert light count to heat map color
float3 LightCountToHeatMap(uint lightCount)
{
    // 0 lights = blue, 10 lights = green, 20+ lights = red
    float t = saturate((float)lightCount / 20.0);

    if (t < 0.25)
        return lerp(float3(0, 0, 1), float3(0, 1, 1), t * 4.0); // Blue to Cyan
    else if (t < 0.5)
        return lerp(float3(0, 1, 1), float3(0, 1, 0), (t - 0.25) * 4.0); // Cyan to Green
    else if (t < 0.75)
        return lerp(float3(0, 1, 0), float3(1, 1, 0), (t - 0.5) * 4.0); // Green to Yellow
    else
        return lerp(float3(1, 1, 0), float3(1, 0, 0), (t - 0.75) * 4.0); // Yellow to Red
}

#endif // USE_TILED_CULLING

//#endif
// ============== BRDF ==============
float3 Diffuse_Lambert(float3 BaseColor)
{
    return BaseColor / PI;
}

float D_GGXNormalDistributionFunction(float NdotH, float alpha)
{
    //specular
    float a2 = alpha * alpha; // aplha = roughness
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float3 F_ShilckApprox(float3 F0, float VdotH)
{
    float m = 1.0 - saturate(VdotH);
    return F0 + (1.0 - F0) * pow(m, 5);
}
float G_Smith(float a2, float NdotV, float NdotL)
{
    float G_SmithV = NdotV + sqrt(NdotV * (NdotV - NdotV * a2) + a2);
    float G_SmithL = NdotL + sqrt(NdotL * (NdotL - NdotL * a2) + a2);
    return (4 * NdotV * NdotL) / (G_SmithV * G_SmithL);
} 

void ComputeBRDF(float3 Li, float3 N, float3 V, float3 L, float atten,
    float3 baseColor, float roughness, float metallic, inout LightAccum acc)
{
    
    float3 H = normalize(L + V);
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));


    // Convert artist-friendly roughness to microfacet alpha
    float alpha = max(roughness * roughness, 1e-3);

    // Compute base reflectance F0 using metallic workflow
    float3 dielectricF0 = float3(1.0, 0.782, 0.0);
    float3 F0 = lerp(dielectricF0, baseColor, saturate(metallic));

    float D = D_GGXNormalDistributionFunction(NdotH, alpha); 
    float3 F = F_ShilckApprox(F0, VdotH);
    float G = G_Smith(alpha, NdotV, NdotL);
    
    
    float denom = max(4.0f * NdotV, 1e-3); 
    float3 lit = Li * NdotL * atten;
    float3 specBRDF = D * F * G / denom;
    // Reduce diffuse as surface becomes metallic
    float3 diffBRDF = Diffuse_Lambert(baseColor) * NdotL;

    
    acc.specular += specBRDF * lit; //  / denom;
    acc.diffuse += diffBRDF * lit; 
}

LightAccum BRDF(float3 cameraWorldPos, float3 worldPos, float3 N, float3 BaseColor, float roughness, float metallic)
{ 
    //PointLightCount
    
    LightAccum acc = (LightAccum) 0;
    
    float3 V = normalize(cameraWorldPos - worldPos);
    
    //Lamber Diffuse
    
    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        FPointLightData pointLight = PointLights[i];
         
        float3 LvecToLight = pointLight.Position.xyz - worldPos;
        float dist = length(LvecToLight);
        float3 L = normalize(LvecToLight);
         
        float range = max(pointLight.Position.w, 1e-3);
        float fall = max(pointLight.Color.w, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);
         
        float3 Li = pointLight.Color.rgb;
        ComputeBRDF(Li, N, V, L, atten, BaseColor, roughness, metallic, acc); 
    } 
    
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
        float fall = max(light.Color.w, 0.001);
        float t = saturate(dist / range);
        float attenDist = pow(saturate(1.0 - t), fall);
        // float attenDist = pow(saturate(1 / (light.AttFactor.x + range * light.AttFactor.y + range * range * light.AttFactor.z)), fall);
         
        // 선형 보간 
        float3 lightDir = normalize(light.Direction.xyz);
        float3 lightToWorld = normalize(-L); // light -> surface
        float cosTheta = dot(lightDir, lightToWorld);
         
        float thetaInner = cos(radians(light.InnerConeAngle));
        float thetaOuter = cos(radians(light.OuterConeAngle));
        
        float atten = saturate(pow((cosTheta - thetaOuter) / max((thetaInner - thetaOuter), 1e-3), light.InAndOutSmooth));
          
        // Diffuse
        float3 Li = light.Color.rgb;
        float NdotL = saturate(dot(N, L));
        
        
        ComputeBRDF(Li, N, V, L, (atten * attenDist), BaseColor, roughness, metallic, acc);
    }
    
    
    // TODO: Direction
    for (int i = 0; i < DirectionalLightCount; ++i)
    {
        FDirectionalLightData light = DirectionalLights[i];
        float3 LightVector = normalize(-light.Direction);

        float3 Li = light.Color.rgb;
        ComputeBRDF(Li, N, V, LightVector, 1.0f, BaseColor, roughness, metallic, acc);
    }
    
    return acc;
}

