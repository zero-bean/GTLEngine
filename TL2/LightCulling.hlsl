// ============================================================================
// LightCulling.hlsl - Tile-based Light Culling Compute Shader (LH version)
// ============================================================================

// Tile configuration
#define TILE_SIZE 32
#define MAX_LIGHTS_PER_TILE 256
#define MAX_POINT_LIGHTS 100
#define MAX_SPOT_LIGHTS 100

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
    float2 NearFar; // LH(+Z forward)
    float2 _pad_culling;
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
// Helper Function : 라이트가 이 타일에 영향을 주는지 판단 (LH 버전)
// ============================================================================
bool IsLightAffectingTile(
    float3 lightPosView, float lightRadius,
    float nearZ, float farZ,
    float4x4 ProjInv,
    float2 ndcMin, float2 ndcMax)
{
    // --- Z-range culling (LH: +Z forward) ---
    if (lightPosView.z + lightRadius < nearZ)
        return false;
    if (lightPosView.z + lightRadius > farZ)
        return false;

    // --- Project to NDC ---
    float projScaleX = 1.0 / ProjInv[0][0];
    float projScaleY = 1.0 / ProjInv[1][1];
    float2 lightNDC;

    // LH(+Z forward): X 정상, Y는 D3D 좌표 보정 (-)
    lightNDC.x = (lightPosView.x * projScaleX / lightPosView.z);
    lightNDC.y = (lightPosView.y * projScaleY / lightPosView.z);

    // 반경 근사 (NDC 스페이스에서)
    float radiusNDC = lightRadius * projScaleX / abs(lightPosView.z);

    // --- Circle vs Rect test ---
    float2 closest;
    closest.x = clamp(lightNDC.x, ndcMin.x, ndcMax.x);
    closest.y = clamp(lightNDC.y, ndcMin.y, ndcMax.y);

    float2 diff = lightNDC - closest;
    float distSq = dot(diff, diff);

    return (distSq <= radiusNDC * radiusNDC);
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

    // -------------------------
    // Step 2. Tile bounds in screen space
    // -------------------------
    uint2 tileMin = groupID.xy * TILE_SIZE;
    uint2 tileMax = min(tileMin + uint2(TILE_SIZE, TILE_SIZE), uint2(ScreenWidth, ScreenHeight));

    // Convert to NDC bounds
    float2 tileScreenMin = float2(tileMin.x, tileMin.y);
    float2 tileScreenMax = float2(tileMax.x, tileMax.y);
    float2 tileNDCMin = (tileScreenMin / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
    float2 tileNDCMax = (tileScreenMax / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
    tileNDCMin.y = -tileNDCMin.y;
    tileNDCMax.y = -tileNDCMax.y;

    float2 ndcMin = float2(min(tileNDCMin.x, tileNDCMax.x), min(tileNDCMin.y, tileNDCMax.y));
    float2 ndcMax = float2(max(tileNDCMin.x, tileNDCMax.x), max(tileNDCMin.y, tileNDCMax.y));

    // -------------------------
    // Step 3. Light test
    // -------------------------
    uint totalLights = (uint) PointLightCount + (uint) SpotLightCount;
    uint numThreads = TILE_SIZE * TILE_SIZE;

    for (uint lightIndex = groupIndex; lightIndex < totalLights; lightIndex += numThreads)
    {
        float3 lightPosWorld;
        float lightRadius;

        if (lightIndex < (uint) PointLightCount)
        {
            lightPosWorld = PointLights[lightIndex].Position.xyz;
            lightRadius = PointLights[lightIndex].Position.w;
        }
        else
        {
            uint spotIndex = lightIndex - (uint) PointLightCount;
            lightPosWorld = SpotLights[spotIndex].Position.xyz;
            lightRadius = SpotLights[spotIndex].Position.w;
        }

        // Transform to view space
        float4 lightPosView4 = mul(float4(lightPosWorld, 1.0f), ViewMatrix);
        float3 lightPosView = lightPosView4.xyz;

        // --- 함수 호출로 대체 ---
        bool intersects = IsLightAffectingTile(
            lightPosView,
            lightRadius,
            NearFar.x, NearFar.y,
            ProjInv,
            ndcMin, ndcMax
        );

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
