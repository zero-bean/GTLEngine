//================================================================================================
// Filename:      LightingCommon.hlsl
// Description:   조명 계산 공통 함수
//                매크로를 통해 GOURAUD, LAMBERT, PHONG 조명 모델 지원
//================================================================================================

// 주의: 이 파일은 다음 파일들 이후에 include 되어야 함:
// - LightStructures.hlsl
// - LightingBuffers.hlsl

//================================================================================================
// Specular 계산 방식 선택
//================================================================================================
// USE_BLINN_PHONG 정의됨: Half-vector 기반 Blinn-Phong (더 빠르고 부드러운 하이라이트)
// USE_BLINN_PHONG 주석 처리: Reflection vector 기반 전통적인 Phong (더 날카로운 하이라이트)
#define USE_BLINN_PHONG 1

//================================================================================================
// 유틸리티 함수
//================================================================================================

// 타일 인덱스 계산 (픽셀 위치로부터)
// SV_POSITION은 픽셀 중심 좌표 (0.5, 0.5 offset)
uint CalculateTileIndex(float4 screenPos)
{
    uint tileX = uint(screenPos.x) / TileSize;
    uint tileY = uint(screenPos.y) / TileSize;
    return tileY * TileCountX + tileX;
}

// 타일별 라이트 인덱스 데이터의 시작 오프셋 계산
uint GetTileDataOffset(uint tileIndex)
{
    // MaxLightsPerTile = 256 (TileLightCuller.h와 일치)
    const uint MaxLightsPerTile = 256;
    return tileIndex * MaxLightsPerTile;
}

//================================================================================================
// 기본 조명 계산 함수
//================================================================================================

// Ambient Light 계산 (OBJ/MTL 표준)
// 공식: Ambient = La × Ka
// 주의: light.Color (La)는 이미 Intensity와 Temperature가 포함됨
// ambientColor: 재질의 Ambient Color (Ka) - Diffuse Color가 아님!
float3 CalculateAmbientLight(FAmbientLightInfo light, float3 ambientColor)
{
    // OBJ/MTL 표준: La × Ka
    return light.Color.rgb * ambientColor;
}

// Diffuse Light 계산 (Lambert)
// 제공된 materialColor 사용 (텍스처가 있으면 포함됨)
// 주의: lightColor는 이미 Intensity가 포함됨 (C++에서 계산)
float3 CalculateDiffuse(float3 lightDir, float3 normal, float4 lightColor, float4 materialColor)
{
    float NdotL = max(dot(normal, lightDir), 0.0f);
    // materialColor를 직접 사용 (이미 텍스처가 포함되어 있으면 포함됨)
    return lightColor.rgb * materialColor.rgb * NdotL;
}

// Specular 색상 결정 매크로
// 각 셰이더에서 include 전에 이 매크로를 정의하여 커스터마이즈 가능
// 기본값: 흰색 스페큘러 (1,1,1)
#ifndef SPECULAR_COLOR
    #define SPECULAR_COLOR float3(1.0f, 1.0f, 1.0f)
#endif

// Specular Light 계산 함수
// USE_BLINN_PHONG 매크로에 따라 Blinn-Phong 또는 전통적인 Phong 방식 사용
// 주의: lightColor는 이미 Intensity가 포함됨 (C++에서 계산)
// 주의: SPECULAR_COLOR 매크로로 specular 색상 제어 (기본: 흰색)
float3 CalculateSpecular(float3 lightDir, float3 normal, float3 viewDir, float4 lightColor, float specularPower)
{
    float3 specularColor = SPECULAR_COLOR;

#ifdef USE_BLINN_PHONG
    // Blinn-Phong 방식: Half-vector 기반 (더 빠르고 부드러운 하이라이트)
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVec), 0.0f);
    float specular = pow(NdotH, specularPower);
    return lightColor.rgb * specularColor * specular;
#else
    // 전통적인 Phong 방식: Reflection vector 기반 (더 날카로운 하이라이트)
    float3 reflectDir = reflect(-lightDir, normal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0f);
    float specular = pow(RdotV, specularPower);
    return lightColor.rgb * specularColor * specular;
#endif
}

//================================================================================================
// 감쇠(Attenuation) 함수
//================================================================================================

// Unreal Engine: Inverse Square Falloff (물리 기반)
// https://docs.unrealengine.com/en-US/BuildingWorlds/LightingAndShadows/PhysicalLightUnits/
float CalculateInverseSquareFalloff(float distance, float attenuationRadius)
{
    // Unreal Engine의 역제곱 감쇠 with 부드러운 윈도우 함수
    float distanceSq = distance * distance;
    float radiusSq = attenuationRadius * attenuationRadius;

    // 기본 역제곱 법칙: I = 1 / (distance^2)
    float basicFalloff = 1.0f / max(distanceSq, 0.01f * 0.01f); // 0으로 나누기 방지

    // attenuationRadius에서 0에 도달하는 부드러운 윈도우 함수 적용
    // 반경 경계에서 급격한 차단을 방지
    float distanceRatio = saturate(distance / attenuationRadius);
    float windowAttenuation = pow(1.0f - pow(distanceRatio, 4.0f), 2.0f);

    return basicFalloff * windowAttenuation;
}

// Unreal Engine: Light Falloff Exponent (예술적 제어)
// 물리적으로 정확하지 않지만 예술적 제어 제공
float CalculateExponentFalloff(float distance, float attenuationRadius, float falloffExponent)
{
    // 정규화된 거리 (라이트 중심에서 0, 반경 경계에서 1)
    float distanceRatio = saturate(distance / attenuationRadius);

    // 단순 지수 기반 감쇠: attenuation = (1 - ratio)^exponent
    // falloffExponent가 곡선을 제어:
    // - exponent = 1: 선형 감쇠
    // - exponent > 1: 더 빠른 감쇠 (더 날카로움)
    // - exponent < 1: 더 느린 감쇠 (더 부드러움)
    float attenuation = pow(1.0f - pow(distanceRatio, 2.0f), max(falloffExponent, 0.1f));

    return attenuation;
}

//================================================================================================
// Shadow Mapping Functions
//================================================================================================

/**
 * Sample SpotLight shadow map
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param shadowMapIndex - Index into shadow map array
 * @param lightSpacePos - Position in light's clip space
 * @param worldNormal - World space normal for slope-scaled bias
 * @param lightDir - Light direction for slope calculation
 */
float SampleSpotLightShadowMap(uint shadowMapIndex, float4 lightSpacePos, float3 worldNormal, float3 lightDir)
{
    // Perspective divide to get NDC coordinates
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert NDC [-1, 1] to texture coordinates [0, 1]
    float2 shadowTexCoord;
    shadowTexCoord.x = projCoords.x * 0.5f + 0.5f;
    shadowTexCoord.y = -projCoords.y * 0.5f + 0.5f; // Flip Y for D3D

    // Check if position is outside shadow map bounds
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f; // Outside shadow map = not shadowed
    }

    // Current depth in light space
    float currentDepth = projCoords.z;

    // Slope-scaled bias: 표면 기울기에 따라 bias 자동 조절
    // 빛과 평행한 표면(NdotL=1.0): 최소 bias
    // 빛과 수직에 가까운 표면(NdotL=0.0): 최대 bias
    float NdotL = saturate(dot(worldNormal, lightDir));
    float bias = lerp(0.00001f, 0.00005f, NdotL);
    currentDepth -= bias;

    // Use comparison sampler for hardware PCF (Percentage Closer Filtering)
    float3 shadowSampleCoord = float3(shadowTexCoord, shadowMapIndex);
    float shadow = g_SpotLightShadowMaps.SampleCmpLevelZero(g_ShadowSampler, shadowSampleCoord, currentDepth);

    return shadow;
}

/**
 * Sample DirectionalLight shadow map
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param shadowMapIndex - Index into shadow map array
 * @param lightSpacePos - Position in light's clip space
 * @param worldNormal - World space normal for slope-scaled bias
 * @param lightDir - Light direction for slope calculation
 */
float SampleDirectionalLightShadowMap(uint shadowMapIndex, float4 lightSpacePos, float3 worldNormal, float3 lightDir)
{
    // Perspective divide to get NDC coordinates
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert NDC [-1, 1] to texture coordinates [0, 1]
    float2 shadowTexCoord;
    shadowTexCoord.x = projCoords.x * 0.5f + 0.5f;
    shadowTexCoord.y = -projCoords.y * 0.5f + 0.5f; // Flip Y for D3D

    // Check if position is outside shadow map bounds
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f; // Outside shadow map = not shadowed
    }

    // Current depth in light space
    float currentDepth = projCoords.z;

    // Slope-scaled bias: 표면 기울기에 따라 bias 자동 조절
    // 빛과 평행한 표면(NdotL=1.0): 최소 bias
    // 빛과 수직에 가까운 표면(NdotL=0.0): 최대 bias
    float NdotL = saturate(dot(worldNormal, lightDir));
    float bias = lerp(0.00001f, 0.00005f, NdotL);
    currentDepth -= bias;

    // Use comparison sampler for hardware PCF (Percentage Closer Filtering)
    float3 shadowSampleCoord = float3(shadowTexCoord, shadowMapIndex);
    float shadow = g_DirectionalLightShadowMaps.SampleCmpLevelZero(g_ShadowSampler, shadowSampleCoord, currentDepth);

    return shadow;
}

/**
 * Sample PointLight shadow map using LightViewProjection matrices
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param light - Point light information (includes 6 VP matrices)
 * @param worldPos - World position
 * @param worldNormal - World space normal for slope-scaled bias
 */
float SamplePointLightShadowMap(FPointLightInfo light, float3 worldPos, float3 worldNormal)
{
    // 1. 라이트에서 픽셀로의 방향 벡터 계산
    float3 lightToPixel = worldPos - light.Position;
    float3 lightDir = normalize(-lightToPixel); // 픽셀에서 라이트로의 방향

    // 2. 방향에 따라 큐브맵 면 선택 (0-5)
    // Cube Map Face 순서: +X(0), -X(1), +Y(2), -Y(3), +Z(4), -Z(5)
    float3 absDir = abs(lightToPixel);
    uint faceIndex = 0;

    if (absDir.x >= absDir.y && absDir.x >= absDir.z)
    {
        // X축이 가장 큼
        faceIndex = lightToPixel.x > 0.0f ? 0 : 1; // +X or -X
    }
    else if (absDir.y >= absDir.z)
    {
        // Y축이 가장 큼
        faceIndex = lightToPixel.y > 0.0f ? 2 : 3; // +Y or -Y
    }
    else
    {
        // Z축이 가장 큼
        faceIndex = lightToPixel.z > 0.0f ? 4 : 5; // +Z or -Z
    }

    // 3. 선택된 LightViewProjection으로 LightSpacePos 계산
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), light.LightViewProjection[faceIndex]);

    // 4. Perspective divide to get NDC coordinates
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // 5. Convert NDC [-1, 1] to texture coordinates [0, 1]
    float2 shadowTexCoord;
    shadowTexCoord.x = projCoords.x * 0.5f + 0.5f;
    shadowTexCoord.y = -projCoords.y * 0.5f + 0.5f; // Flip Y for D3D

    // 6. Check if position is outside shadow map bounds
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f; // Outside shadow map = not shadowed
    }

    // 7. Current depth in light space
    float currentDepth = projCoords.z;

    // 8. Slope-scaled bias: 표면 기울기에 따라 bias 자동 조절
    // 빛과 평행한 표면(NdotL=1.0): 최소 bias
    // 빛과 수직에 가까운 표면(NdotL=0.0): 최대 bias
    float NdotL = saturate(dot(worldNormal, lightDir));
    float bias = lerp(0.00001f, 0.00005f, NdotL);
    currentDepth -= bias;

    // 9. Use comparison sampler for hardware PCF
    // TextureCubeArray 샘플링: direction + array index 사용
    float4 cubeSampleCoord = float4(lightToPixel, light.ShadowMapIndex);
    float shadow = g_PointLightShadowCubeMaps.SampleCmpLevelZero(g_ShadowSampler, cubeSampleCoord, currentDepth);

    return shadow;
}

//================================================================================================
// 통합 조명 계산 함수
//================================================================================================

// Directional Light 계산 (Diffuse + Specular + Shadow)
float3 CalculateDirectionalLight(FDirectionalLightInfo light, float3 worldPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower)
{
    // Light.Direction이 영벡터인 경우, 정규화 문제 방지
    if (all(light.Direction == float3(0.0f, 0.0f, 0.0f)))
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 lightDir = normalize(-light.Direction);

    // Shadow mapping (if this light casts shadows)
    float shadow = 1.0f;
    if (light.bCastShadow && light.ShadowMapIndex != 0xFFFFFFFF)
    {
        // Transform world position to light space
        float4 lightSpacePos = mul(float4(worldPos, 1.0f), light.LightViewProjection);
        shadow = SampleDirectionalLightShadowMap(light.ShadowMapIndex, lightSpacePos, normal, lightDir);
    }

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor) * shadow;

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower) * shadow;
    }

    return diffuse + specular;
}

// Point Light 계산 (Diffuse + Specular with Attenuation and Falloff + Shadow)
float3 CalculatePointLight(FPointLightInfo light, float3 worldPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower)
{
    float3 lightVec = light.Position - worldPos;
    float distance = length(lightVec);

    // epsilon으로 0으로 나누기 방지
    distance = max(distance, 0.0001f);
    float3 lightDir = lightVec / distance;

    // Falloff 모드에 따라 감쇠 계산
    float attenuation;
    if (light.bUseInverseSquareFalloff)
    {
        // 물리 기반 역제곱 감쇠 (Unreal Engine 스타일)
        attenuation = CalculateInverseSquareFalloff(distance, light.AttenuationRadius);
    }
    else
    {
        // 더 큰 제어를 위한 예술적 지수 기반 감쇠
        attenuation = CalculateExponentFalloff(distance, light.AttenuationRadius, light.FalloffExponent);
    }

    // Shadow mapping (if this light casts shadows)
    float shadow = 1.0f;
    if (light.bCastShadow && light.ShadowMapIndex != 0xFFFFFFFF)
    {
        // LightViewProjection 기반 Shadow 샘플링
        shadow = SamplePointLightShadowMap(light, worldPos, normal);
    }

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor) * attenuation * shadow;

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower) * attenuation * shadow;
    }

    return diffuse + specular;
}

// Spot Light 계산 (Diffuse + Specular with Attenuation and Cone)
float3 CalculateSpotLight(FSpotLightInfo light, float3 worldPos, float3 normal, float3 viewDir, float4 materialColor, bool includeSpecular, float specularPower)
{
    float3 lightVec = light.Position - worldPos;
    float distance = length(lightVec);

    // epsilon으로 0으로 나누기 방지
    distance = max(distance, 0.0001f);
    float3 lightDir = lightVec / distance;
    float3 spotDir = normalize(light.Direction);

    // Spot 원뿔 감쇠 (각도 공간에서 보간하여 중간에서 급격하게 변함)
    float cosAngle = dot(-lightDir, spotDir);

    // cosine 공간 대신 각도 공간에서 보간 (균등한 감쇠를 위해)
    float angle = degrees(acos(saturate(cosAngle)));  // 현재 각도 (도 단위)

    // inner cone에서 1, outer cone에서 0, 중간에서 급격하게 변함
    float spotAttenuation = 1.0 - smoothstep(light.InnerConeAngle, light.OuterConeAngle, angle);

    // Falloff 모드에 따라 거리 감쇠 계산
    float distanceAttenuation;
    if (light.bUseInverseSquareFalloff)
    {
        // 물리 기반 역제곱 감쇠 (Unreal Engine 스타일)
        distanceAttenuation = CalculateInverseSquareFalloff(distance, light.AttenuationRadius);
    }
    else
    {
        // 더 큰 제어를 위한 예술적 지수 기반 감쇠
        distanceAttenuation = CalculateExponentFalloff(distance, light.AttenuationRadius, light.FalloffExponent);
    }

    // 두 감쇠를 결합
    float attenuation = distanceAttenuation * spotAttenuation;

    // Shadow mapping (if this light casts shadows)
    float shadow = 1.0f;
    if (light.bCastShadow && light.ShadowMapIndex != 0xFFFFFFFF)
    {
        // Transform world position to light space
        float4 lightSpacePos = mul(float4(worldPos, 1.0f), light.LightViewProjection);
        shadow = SampleSpotLightShadowMap(light.ShadowMapIndex, lightSpacePos, normal, lightDir);
    }

    // Diffuse (light.Color는 이미 Intensity 포함)
    float3 diffuse = CalculateDiffuse(lightDir, normal, light.Color, materialColor) * attenuation * shadow;

    // Specular (선택사항)
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (includeSpecular)
    {
        specular = CalculateSpecular(lightDir, normal, viewDir, light.Color, specularPower) * attenuation * shadow;
    }

    return diffuse + specular;
}

//================================================================================================
// 매크로 기반 조명 모델 헬퍼
//================================================================================================

// 조명 모델에 따라 적절한 파라미터 자동 설정
#ifdef LIGHTING_MODEL_GOURAUD
    #define LIGHTING_INCLUDE_SPECULAR true
    #define LIGHTING_NEED_VIEWDIR true
#elif defined(LIGHTING_MODEL_LAMBERT)
    #define LIGHTING_INCLUDE_SPECULAR false
    #define LIGHTING_NEED_VIEWDIR false
#elif defined(LIGHTING_MODEL_PHONG)
    #define LIGHTING_INCLUDE_SPECULAR true
    #define LIGHTING_NEED_VIEWDIR true
#else
    // 조명 모델 미정의 - 기본값 제공
    #define LIGHTING_INCLUDE_SPECULAR false
    #define LIGHTING_NEED_VIEWDIR false
#endif

//================================================================================================
// 타일 컬링 기반 전체 조명 계산 (매크로 자동 대응)
//================================================================================================

// 모든 라이트(Point + Spot) 계산 - 타일 컬링 지원
// 매크로에 따라 자동으로 specular on/off
// 주의: 이 헬퍼 함수는 ambient에 baseColor 사용 (Ka = Kd 가정)
//       OBJ/MTL 재질의 경우, Material.AmbientColor로 CalculateAmbientLight를 별도로 호출할 것
float3 CalculateAllLights(
    float3 worldPos,
    float3 normal,
    float3 viewDir,        // LAMBERT에서는 사용 안 함
    float4 baseColor,
    float specularPower,
    float4 screenPos)      // 타일 컬링용
{
    float3 litColor = float3(0, 0, 0);

    // Ambient (비재질 오브젝트는 Ka = Kd 가정)
    litColor += CalculateAmbientLight(AmbientLight, baseColor.rgb);

    // Directional (with shadow support)
    litColor += CalculateDirectionalLight(
        DirectionalLight,
        worldPos,  // worldPos 추가 (shadow 계산용)
        normal,
        viewDir,  // LAMBERT에서는 무시됨
        baseColor,
        LIGHTING_INCLUDE_SPECULAR,  // 매크로에 따라 자동 설정
        specularPower
    );

    // Point + Spot with 타일 컬링
    if (bUseTileCulling)
    {
        uint tileIndex = CalculateTileIndex(screenPos);
        uint tileDataOffset = GetTileDataOffset(tileIndex);
        uint lightCount = g_TileLightIndices[tileDataOffset];

        for (uint i = 0; i < lightCount; i++)
        {
            uint packedIndex = g_TileLightIndices[tileDataOffset + 1 + i];
            uint lightType = (packedIndex >> 16) & 0xFFFF;
            uint lightIdx = packedIndex & 0xFFFF;

            if (lightType == 0)  // Point Light
            {
                litColor += CalculatePointLight(
                    g_PointLightList[lightIdx],
                    worldPos,
                    normal,
                    viewDir,
                    baseColor,
                    LIGHTING_INCLUDE_SPECULAR,  // 매크로 자동 대응
                    specularPower
                );
            }
            else if (lightType == 1)  // Spot Light
            {
                litColor += CalculateSpotLight(
                    g_SpotLightList[lightIdx],
                    worldPos,
                    normal,
                    viewDir,
                    baseColor,
                    LIGHTING_INCLUDE_SPECULAR,  // 매크로 자동 대응
                    specularPower
                );
            }
        }
    }
    else
    {
        // 모든 라이트 순회 (타일 컬링 비활성화)
        for (int i = 0; i < PointLightCount; i++)
        {
            litColor += CalculatePointLight(
                g_PointLightList[i],
                worldPos,
                normal,
                viewDir,
                baseColor,
                LIGHTING_INCLUDE_SPECULAR,
                specularPower
            );
        }

        for (int j = 0; j < SpotLightCount; j++)
        {
            litColor += CalculateSpotLight(
                g_SpotLightList[j],
                worldPos,
                normal,
                viewDir,
                baseColor,
                LIGHTING_INCLUDE_SPECULAR,
                specularPower
            );
        }
    }

    return litColor;
}
