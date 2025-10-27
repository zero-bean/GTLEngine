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

//------------------------------------------------------------------------------------------------
// Paraboloid 유틸리티 함수
//------------------------------------------------------------------------------------------------

/**
 * World Position을 Paraboloid UV 좌표와 깊이로 변환
 *
 * @param worldPos - 월드 공간 위치
 * @param lightPos - 라이트 위치
 * @param attenuationRadius - 라이트 감쇠 반경
 * @param nearPlane - Near clipping plane
 * @param bFrontHemisphere - true = 전면 반구 (+Z), false = 후면 반구 (-Z)
 * @param outUV - (출력) Paraboloid UV 좌표 [0, 1]
 * @param outDepth - (출력) 선형 깊이 [0, 1]
 * @return 유효한 샘플인지 여부 (범위 내 = true, 범위 밖 = false)
 */
bool WorldPosToParaboloidUV(
    float3 worldPos,
    float3 lightPos,
    float attenuationRadius,
    float nearPlane,
    bool bFrontHemisphere,
    out float2 outUV,
    out float outDepth)
{
    // 1. Light View Space로 변환 (라이트 중심 기준)
    float3 lightVec = worldPos - lightPos;
    float distance = length(lightVec);

    // epsilon으로 0으로 나누기 방지
    if (distance < 0.0001f)
    {
        outUV = float2(0.5f, 0.5f);
        outDepth = 0.0f;
        return false;
    }

    float3 viewDir = lightVec / distance;

    // 2. 전면/후면 반구 선택에 따라 Z 부호 조정
    // Z축: Forward = +Y, Up = +Z, Right = +X (Z-Up Left-Handed)
    // Paraboloid는 기본적으로 +Z를 forward로 가정
    // 따라서 viewDir을 Z축 기준으로 재정렬 필요

    // Z-Up 좌표계에서 Y가 forward이므로, viewDir.y를 z로 사용
    float3 paraboloidDir = float3(viewDir.x, viewDir.z, viewDir.y);

    if (!bFrontHemisphere)
    {
        // 후면 반구: Z를 반전시켜 -Z 방향이 forward가 되도록
        paraboloidDir.z = -paraboloidDir.z;
    }

    // 3. Paraboloid 투영 공식
    float denominator = 1.0f + paraboloidDir.z;

    // Z가 음수면 해당 반구 범위를 벗어난 것 (clipping)
    if (denominator <= 0.0001f)
    {
        outUV = float2(0.5f, 0.5f);
        outDepth = 1.0f;
        return false;
    }

    // 4. Paraboloid UV 좌표 계산 (NDC: -1 ~ 1)
    float2 paraboloidNDC;
    paraboloidNDC.x = paraboloidDir.x / denominator;
    paraboloidNDC.y = paraboloidDir.y / denominator;

    // 5. NDC [-1, 1]을 UV [0, 1]로 변환
    outUV.x = paraboloidNDC.x * 0.5f + 0.5f;
    outUV.y = paraboloidNDC.y * 0.5f + 0.5f;  // Y 플립 (DirectX)

    // 6. 선형 깊이 계산 [0, 1]
    outDepth = saturate((distance - nearPlane) / (attenuationRadius - nearPlane));

    // 7. UV 범위 체크 (Paraboloid는 원형 영역)
    // UV가 [0, 1] 범위를 벗어나면 유효하지 않음
    if (outUV.x < 0.0f || outUV.x > 1.0f || outUV.y < 0.0f || outUV.y > 1.0f)
    {
        return false;
    }

    return true;
}

/**
 * Sample SpotLight shadow map
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param shadowMapIndex - Index into shadow map array
 * @param lightSpacePos - Position in light's clip space
 */
float SampleSpotLightShadowMap(uint shadowMapIndex, float4 lightSpacePos)
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

    // Bias to prevent shadow acne
    float bias = 0.00001f;
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
 */
float SampleDirectionalLightShadowMap(uint shadowMapIndex, float4 lightSpacePos)
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

    // Bias to prevent shadow acne (can be adjusted for directional lights)
    float bias = 0.005f;
    currentDepth -= bias;

    // Use comparison sampler for hardware PCF (Percentage Closer Filtering)
    float3 shadowSampleCoord = float3(shadowTexCoord, shadowMapIndex);
    float shadow = g_DirectionalLightShadowMaps.SampleCmpLevelZero(g_ShadowSampler, shadowSampleCoord, currentDepth);

    return shadow;
}

/**
 * Sample PointLight shadow map (Cube Map)
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param shadowMapIndex - Index into cube shadow map array
 * @param worldPos - World position
 * @param lightPos - Light position
 * @param attenuationRadius - Light attenuation radius
 * @param nearPlane - Near clipping plane
 */
float SamplePointLightShadowCube(
    uint shadowMapIndex,
    float3 worldPos,
    float3 lightPos,
    float attenuationRadius,
    float nearPlane)
{
    // 1. 라이트에서 픽셀로의 방향 벡터 계산 (Z-Up World Space)
    float3 lightToPixel = worldPos - lightPos;
    float distance = length(lightToPixel);

    // 2. 선형 거리를 비선형 깊이로 변환 (Perspective Projection)
    // Perspective projection formula: Z_ndc = (far / (far - near)) - (far * near) / ((far - near) * Z_view)
    // 여기서 Z_view = distance (view space에서의 깊이)
    float far = attenuationRadius;
    float near = nearPlane;
    float nonlinearDepth = (far / (far - near)) - (far * near) / ((far - near) * distance);

    // 3. [0, 1] 범위로 정규화 (DirectX의 경우 이미 [0, 1] 범위겠지만 보험)
    float currentDepth = saturate(nonlinearDepth);

    // 4. Bias to prevent shadow acne
    float bias = 0.005f;
    currentDepth -= bias;

    // 5. TextureCubeArray 샘플링
    // SampleCmpLevelZero: cube direction + array index
    float4 sampleCoord = float4(lightToPixel, shadowMapIndex);
    float shadow = g_PointLightShadowCubeMaps.SampleCmpLevelZero(g_ShadowSampler, sampleCoord, currentDepth);

    return shadow;
}

/**
 * Sample PointLight shadow map (Paraboloid)
 * Returns 0.0 if in shadow, 1.0 if lit
 *
 * @param shadowMapIndex - Index into paraboloid shadow map array
 * @param worldPos - World position
 * @param lightPos - Light position
 * @param attenuationRadius - Light attenuation radius
 * @param nearPlane - Near clipping plane
 */
float SamplePointLightShadowParaboloid(
    uint shadowMapIndex,
    float3 worldPos,
    float3 lightPos,
    float attenuationRadius,
    float nearPlane)
{
    // 1. 라이트에서 픽셀로의 방향 벡터 계산
    float3 lightToPixel = worldPos - lightPos;
    float distance = length(lightToPixel);

    // epsilon으로 0으로 나누기 방지
    if (distance < 0.0001f)
    {
        return 1.0f; // 라이트 중심에 있으면 그림자 없음
    }

    float3 viewDir = lightToPixel / distance;

    // 2. 전면/후면 반구 선택
    bool bFrontHemisphere = (viewDir.z > 0.0f);
    uint hemisphereOffset = bFrontHemisphere ? 0 : 1;

    // 3. Paraboloid UV 계산
    float2 paraboloidUV;
    float paraboloidDepth;
    bool bValidSample = WorldPosToParaboloidUV(
        worldPos,
        lightPos,
        attenuationRadius,
        nearPlane,
        bFrontHemisphere,
        paraboloidUV,
        paraboloidDepth);

    // 유효하지 않은 샘플 (범위 밖)
    if (!bValidSample)
    {
        return 1.0f; // 범위 밖은 그림자 없음
    }

    // 4. Bias to prevent shadow acne
    float bias = 0.00001f;
    float currentDepth = paraboloidDepth - bias;

    // 5. Texture2DArray 샘플링
    // ArrayIndex = (LightIndex * 2) + HemisphereIndex
    uint arrayIndex = (shadowMapIndex * 2) + hemisphereOffset;
    float3 sampleCoord = float3(paraboloidUV, arrayIndex);
    float shadow = g_PointLightShadowParaboloidMaps.SampleCmpLevelZero(g_ShadowSampler, sampleCoord, currentDepth);

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
        shadow = SampleDirectionalLightShadowMap(light.ShadowMapIndex, lightSpacePos);
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
        // Near plane (하드코딩, ShadowViewProjection.h의 값과 일치해야 함)
        const float nearPlane = 0.01f;

        // Paraboloid 또는 Cube Map 선택
        if (light.bUseParaboloidMap)
        {
            shadow = SamplePointLightShadowParaboloid(
                light.ShadowMapIndex,
                worldPos,
                light.Position,
                light.AttenuationRadius,
                nearPlane);
        }
        else
        {
            shadow = SamplePointLightShadowCube(
                light.ShadowMapIndex,
                worldPos,
                light.Position,
                light.AttenuationRadius,
                nearPlane);
        }
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
        shadow = SampleSpotLightShadowMap(light.ShadowMapIndex, lightSpacePos);
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
