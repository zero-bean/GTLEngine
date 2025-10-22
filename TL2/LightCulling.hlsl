// ============================================================================
// LightCulling.hlsl - Tile-based Light Culling Compute Shader (LH version)
// ============================================================================

// Tile configuration
#define TILE_SIZE 32
#define MAX_LIGHTS_PER_TILE 256
#define MAX_POINT_LIGHTS 100
#define MAX_SPOT_LIGHTS 100
#define LIGHT_RADIUS_MARGIN 1.1f  // 10% margin to prevent edge clipping

// -------------------------------
// Light structures
// -------------------------------
struct FPointLight
{
    float4 Position; // xyz = position, w = radius
    float4 Color; // rgb = color, a = intensity
    float FallOff;
    float3 _pad;
};

struct FSpotLight
{
    float4 Position; // xyz = position, w = radius
    float4 Color; // rgb = color, a = intensity
    float FallOff;
    float InnerConeAngle;
    float OuterConeAngle;
    float InAndOutSmooth;
    float3 Direction;
    float AttFactor;
};

// -------------------------------
// Buffers
// -------------------------------
cbuffer PointLightBuffer : register(b9)
{
    int PointLightCount;
    float3 _pad_pointlight;
    FPointLight PointLights[MAX_POINT_LIGHTS];
}

cbuffer SpotLightBuffer : register(b13)
{
    int SpotLightCount;
    float3 _pad_spotlight;
    FSpotLight SpotLights[MAX_SPOT_LIGHTS];
}

cbuffer CullingCB : register(b0)
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint NumTilesX;
    uint NumTilesY;
    uint bIsOrthographic;
    float2 NearFar; // LH(+Z forward)
    float _pad_culling;
}

cbuffer ViewMatrixCB : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjInv;
}

// -------------------------------
// Resources
// -------------------------------
Texture2D<float> DepthTexture : register(t0);
RWStructuredBuffer<uint> LightIndexList : register(u0);
RWStructuredBuffer<uint2> TileOffsetCount : register(u1);

// -------------------------------
// Shared memory
// -------------------------------
groupshared uint TileLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint TileLightCount;
groupshared uint TileMinDepthInt;
groupshared uint TileMaxDepthInt;

// ============================================================================
// Helper Function : 포인트 라이트가 이 타일에 영향을 주는지 판단 (LH 버전)
// ============================================================================
bool IsPointLightAffectingTile(
    float3 lightPosView, float lightRadius,
    float nearZ, float farZ,
    float4x4 ProjInv,
    float2 ndcMin, float2 ndcMax)
{
    // Apply margin to prevent edge clipping
    float effectiveRadius = lightRadius * LIGHT_RADIUS_MARGIN;

    // --- Z-range culling (LH: +Z forward) ---
    if (lightPosView.z + effectiveRadius < nearZ)
        return false;
    if (lightPosView.z - effectiveRadius > farZ)
        return false;

    // --- Project to NDC ---
    float projScaleX = 1.0 / ProjInv[0][0];
    float projScaleY = 1.0 / ProjInv[1][1];
    float2 lightNDC;

    if (bIsOrthographic == 1)
    {
        lightNDC.x = lightPosView.x * projScaleX;
        lightNDC.y = lightPosView.y * projScaleY;
    }
    else
    {
        lightNDC.x = (lightPosView.x * projScaleX / lightPosView.z);
        lightNDC.y = (lightPosView.y * projScaleY / lightPosView.z);
    }

    // Calculate ellipse radii in NDC
    float2 radiusNDC;
    if (bIsOrthographic == 1)
    {
        radiusNDC.x = effectiveRadius * projScaleX;
        radiusNDC.y = effectiveRadius * projScaleY;
    }
    else
    {
        radiusNDC.x = effectiveRadius * projScaleX / abs(lightPosView.z);
        radiusNDC.y = effectiveRadius * projScaleY / abs(lightPosView.z);
    }

    // Avoid division by zero
    if (radiusNDC.x < 1e-6f || radiusNDC.y < 1e-6f)
        return false;

    // --- Ellipse vs Rect test ---
    // Scale the coordinate system so the ellipse becomes a unit circle.
    float2 scaledLightNDC = lightNDC / radiusNDC;
    float2 scaledNdcMin = ndcMin / radiusNDC;
    float2 scaledNdcMax = ndcMax / radiusNDC;

    // Find the closest point on the scaled AABB to the scaled light center.
    float2 closest;
    closest.x = clamp(scaledLightNDC.x, scaledNdcMin.x, scaledNdcMax.x);
    closest.y = clamp(scaledLightNDC.y, scaledNdcMin.y, scaledNdcMax.y);

    // Check if the distance from the scaled center to the closest point is within the unit circle.
    float2 diff = scaledLightNDC - closest;
    float distSq = dot(diff, diff);

    return (distSq <= 1.0);
}

// ============================================================================
// Helper Function : 스포트 라이트가 이 타일에 영향을 주는지 판단 (LH 버전)
// ============================================================================
bool IsSpotLightAffectingTile(
    float3 lightPosView, float3 lightDirView, float lightRadius,
    float outerConeAngle,
    float nearZ, float farZ,
    float4x4 ProjInv,
    float2 ndcMin, float2 ndcMax)
{
    // Apply margin to prevent edge clipping
    float effectiveRadius = lightRadius * LIGHT_RADIUS_MARGIN;

    // --- Z-range culling (LH: +Z forward) ---
    if (lightPosView.z + effectiveRadius < nearZ)
        return false;
    if (lightPosView.z - effectiveRadius > farZ)
        return false;

    // --- 타일의 8개 코너 포인트를 View Space로 복원 ---
    float projScaleX = 1.0 / ProjInv[0][0];
    float projScaleY = 1.0 / ProjInv[1][1];

    // 타일의 NDC 경계
    float2 ndcCorners[4];
    ndcCorners[0] = float2(ndcMin.x, ndcMin.y);
    ndcCorners[1] = float2(ndcMax.x, ndcMin.y);
    ndcCorners[2] = float2(ndcMin.x, ndcMax.y);
    ndcCorners[3] = float2(ndcMax.x, ndcMax.y);

    float tileDepths[2];
    tileDepths[0] = nearZ;
    tileDepths[1] = farZ;

    // 스포트라이트 원뿔 각도의 코사인 값
    float cosOuterAngle = cos(outerConeAngle);

    // 타일과 스포트라이트 원뿔의 교차 테스트
    bool anyCornerInCone = false;

    // 타일의 8개 코너 포인트 체크 (4개 코너 x 2개 깊이)
    for (int d = 0; d < 2; ++d)
    {
        float depth = tileDepths[d];

        for (int c = 0; c < 4; ++c)
        {
            // NDC에서 View Space로 복원
            float3 cornerView;
            cornerView.x = ndcCorners[c].x * depth / projScaleX;
            cornerView.y = ndcCorners[c].y * depth / projScaleY;
            cornerView.z = depth;

            // 라이트로부터의 방향 벡터
            float3 toCorner = cornerView - lightPosView;
            float distToCorner = length(toCorner);

            // 범위 체크 - with margin
            if (distToCorner > effectiveRadius)
                continue;

            // 원뿔 각도 체크
            float3 dirToCorner = toCorner / distToCorner;
            float cosAngle = dot(dirToCorner, lightDirView);

            if (cosAngle >= cosOuterAngle)
            {
                anyCornerInCone = true;
                break;
            }
        }

        if (anyCornerInCone)
            break;
    }

    if (anyCornerInCone)
        return true;

    // --- 타일 중심이 원뿔 안에 있는지 체크 ---
    float2 ndcCenter = (ndcMin + ndcMax) * 0.5;
    float centerDepth = (nearZ + farZ) * 0.5;

    float3 centerView;
    centerView.x = ndcCenter.x * centerDepth / projScaleX;
    centerView.y = ndcCenter.y * centerDepth / projScaleY;
    centerView.z = centerDepth;

    float3 toCenter = centerView - lightPosView;
    float distToCenter = length(toCenter);

    if (distToCenter <= effectiveRadius)
    {
        float3 dirToCenter = toCenter / distToCenter;
        float cosAngle = dot(dirToCenter, lightDirView);

        if (cosAngle >= cosOuterAngle)
            return true;
    }

    // --- 라이트 위치가 타일 프러스텀 안에 있는지 체크 ---
    float2 lightNDC;
    lightNDC.x = (lightPosView.x * projScaleX / lightPosView.z);
    lightNDC.y = (lightPosView.y * projScaleY / lightPosView.z);

    if (lightNDC.x >= ndcMin.x && lightNDC.x <= ndcMax.x &&
        lightNDC.y >= ndcMin.y && lightNDC.y <= ndcMax.y &&
        lightPosView.z >= nearZ && lightPosView.z <= farZ)
    {
        return true;
    }

    return false;
}

// ============================================================================
// Main Compute Shader
// ============================================================================
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CS_LightCulling(
    uint3 groupID : SV_GroupID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint groupIndex : SV_GroupIndex)
{
    // -------------------------
    // Init shared memory
    // -------------------------
    if (groupIndex == 0)
    {
        TileLightCount = 0;
        TileMinDepthInt = asuint(10000.0f); // LH(+Z): z는 양수
        TileMaxDepthInt = asuint(-10000.0f);
    }
    GroupMemoryBarrierWithGroupSync();

    // -------------------------
    // Step 1. Tile min/max depth
    // -------------------------
    uint2 pixelPos = groupID.xy * TILE_SIZE + groupThreadID.xy;

    if (pixelPos.x < ScreenWidth && pixelPos.y < ScreenHeight)
    {
        float depth = DepthTexture.Load(int3(pixelPos, 0));

        float2 ndc;
        ndc.x = (pixelPos.x / (float) ScreenWidth) * 2.0 - 1.0;
        ndc.y = -((pixelPos.y / (float) ScreenHeight) * 2.0 - 1.0);

        float4 clipPos = float4(ndc, depth, 1.0);
        float4 viewPos = mul(clipPos, ProjInv);
        viewPos /= viewPos.w;

        uint depthInt = asuint(viewPos.z);
        InterlockedMin(TileMinDepthInt, depthInt);
        InterlockedMax(TileMaxDepthInt, depthInt);
    }
    GroupMemoryBarrierWithGroupSync();

    float TileMinDepth = asfloat(TileMinDepthInt);
    float TileMaxDepth = asfloat(TileMaxDepthInt);

    // 빈 타일 처리: 지오메트리가 없는 타일은 카메라 Near/Far 사용
    if (TileMinDepth >= 10000.0f || TileMaxDepth <= -10000.0f)
    {
        TileMinDepth = NearFar.x;
        TileMaxDepth = NearFar.y;
    }

    // -------------------------
    // Step 2. Tile bounds in screen space
    // -------------------------
    uint2 tileMin = groupID.xy * TILE_SIZE;
    uint2 tileMax = min(tileMin + uint2(TILE_SIZE, TILE_SIZE), uint2(ScreenWidth, ScreenHeight));

    // Convert to NDC bounds
    float2 tileScreenMin = float2(tileMin.x, tileMin.y);
    float2 tileScreenMax = float2(tileMax.x, tileMax.y);

    // Convert to NDC range [-1, 1]
    float2 tileNDCMinRaw = (tileScreenMin / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
    float2 tileNDCMaxRaw = (tileScreenMax / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;

    // Build final NDC bounds, flipping Y and ensuring min < max
    float2 ndcMin, ndcMax;
    ndcMin.x = tileNDCMinRaw.x;
    ndcMax.x = tileNDCMaxRaw.x;

    // Screen Y is 0 at top, NDC Y is 1 at top.
    // So, use tileNDCMaxRaw for ndcMin.y and tileNDCMinRaw for ndcMax.y after flipping.
    ndcMin.y = -tileNDCMaxRaw.y;
    ndcMax.y = -tileNDCMinRaw.y;

    // -------------------------
    // Step 3. Light test
    // -------------------------
    uint totalLights = (uint) PointLightCount + (uint) SpotLightCount;
    uint numThreads = TILE_SIZE * TILE_SIZE;

    for (uint lightIndex = groupIndex; lightIndex < totalLights; lightIndex += numThreads)
    {
        bool intersects = false;

        // --- 포인트 라이트 컬링 ---
        if (lightIndex < (uint) PointLightCount)
        {
            float3 lightPosWorld = PointLights[lightIndex].Position.xyz;
            float lightRadius = PointLights[lightIndex].Position.w;

            // Transform to view space
            float4 lightPosView4 = mul(float4(lightPosWorld, 1.0f), ViewMatrix);
            float3 lightPosView = lightPosView4.xyz;

            intersects = IsPointLightAffectingTile(
                lightPosView,
                lightRadius,
                TileMinDepth, TileMaxDepth,
                ProjInv,
                ndcMin, ndcMax
            );
        }
        // --- 스포트 라이트 컬링 ---
        else
        {
            uint spotIndex = lightIndex - (uint) PointLightCount;
            float3 lightPosWorld = SpotLights[spotIndex].Position.xyz;
            float lightRadius = SpotLights[spotIndex].Position.w;
            float3 lightDirWorld = SpotLights[spotIndex].Direction;
            float outerConeAngle = SpotLights[spotIndex].OuterConeAngle;

            // Transform to view space
            float4 lightPosView4 = mul(float4(lightPosWorld, 1.0f), ViewMatrix);
            float3 lightPosView = lightPosView4.xyz;

            // Direction은 벡터이므로 w=0으로 변환
            float4 lightDirView4 = mul(float4(lightDirWorld, 0.0f), ViewMatrix);
            float3 lightDirView = normalize(lightDirView4.xyz);

            intersects = IsSpotLightAffectingTile(
                lightPosView,
                lightDirView,
                lightRadius,
                outerConeAngle,
                TileMinDepth, TileMaxDepth,
                ProjInv,
                ndcMin, ndcMax
            );
        }

        // --- 타일에 영향 있는 라이트만 추가 ---
        if (intersects)
        {
            uint idx;
            InterlockedAdd(TileLightCount, 1, idx);

            if (idx < MAX_LIGHTS_PER_TILE)
            {
                TileLightIndices[idx] = lightIndex;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // -------------------------
    // Step 4. Write results
    // -------------------------
    if (groupIndex == 0)
    {
        uint tileIndex = groupID.y * NumTilesX + groupID.x;
        uint lightCount = min(TileLightCount, MAX_LIGHTS_PER_TILE);

        uint offset;
        InterlockedAdd(LightIndexList[0], lightCount, offset);

        TileOffsetCount[tileIndex] = uint2(offset + 1, lightCount);

        for (uint i = 0; i < lightCount; ++i)
        {
            LightIndexList[offset + 1 + i] = TileLightIndices[i];
        }
    }
}
