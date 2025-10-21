// LightCulling.hlsl - Tile-based Light Culling Compute Shader

// Tile configuration
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 256
#define MAX_POINT_LIGHTS 100
#define MAX_SPOT_LIGHTS 100

// Point Light structure (from Light.hlsli)
struct FPointLight
{
    float4 Position; // xyz = position, w = radius
    float4 Color;    // rgb = color, a = intensity
    float FallOff;
    float3 _pad;
};

// Spot Light structure (from Light.hlsli)
struct FSpotLight
{
    float4 Position; // xyz = position, w = radius
    float4 Color;    // rgb = color, a = intensity
    float FallOff;
    float InnerConeAngle;
    float OuterConeAngle;
    float InAndOutSmooth;
    float3 Direction;
    float AttFactor;
};

// Light buffers (from Light.hlsli)
// Note: Must match CBufferTypes.h slots - PointLight=b9, SpotLight=b13
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

// Screen and camera info
cbuffer CullingCB : register(b0)
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint NumTilesX;
    uint NumTilesY;
    float2 NearFar; // x = near, y = far
    float2 _pad_culling;
}

// View matrix for transforming lights to view space
cbuffer ViewMatrixCB : register(b1)
{
    row_major float4x4 ViewMatrix;
}

// Depth buffer input
Texture2D<float> DepthTexture : register(t0);

// Output buffers
RWStructuredBuffer<uint> LightIndexList : register(u0);
RWStructuredBuffer<uint2> TileOffsetCount : register(u1);

// Shared memory for tile
groupshared uint TileLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint TileLightCount;
groupshared uint TileMinDepthInt;
groupshared uint TileMaxDepthInt;

// Simple sphere-AABB intersection (conservative test)
bool SphereIntersectsTileAABB(float3 lightPosView, float lightRadius,
                              float2 tileMin, float2 tileMax,
                              float minDepth, float maxDepth)
{
    // Build AABB in view space
    // In view space: X/Y from screen, Z from depth
    float3 aabbMin = float3(tileMin, minDepth);
    float3 aabbMax = float3(tileMax, maxDepth);

    // Find closest point on AABB to sphere center
    float3 closest = clamp(lightPosView, aabbMin, aabbMax);

    // Check distance
    float3 diff = lightPosView - closest;
    float distSq = dot(diff, diff);

    return distSq <= (lightRadius * lightRadius);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CS_LightCulling(
    uint3 groupID : SV_GroupID,
    uint3 groupThreadID : SV_GroupThreadID,
    uint groupIndex : SV_GroupIndex)
{
    // Initialize shared memory
    if (groupIndex == 0)
    {
        TileLightCount = 0;
        TileMinDepthInt = 0x7F7FFFFF; // Max float as uint (positive)
        TileMaxDepthInt = 0;
    }
    if (groupIndex == 0 && groupID.x == 0 && groupID.y == 0)
    {
        printf("PointLightCount=%d SpotLightCount=%d TileMinDepth=%f TileMaxDepth=%f\n",
           PointLightCount, SpotLightCount, TileMinDepthInt, TileMaxDepthInt);
    }
    GroupMemoryBarrierWithGroupSync();

    // Calculate screen-space pixel coordinates
    uint2 pixelPos = groupID.xy * TILE_SIZE + groupThreadID.xy;

    // Step 1: Compute min/max depth for this tile
    if (pixelPos.x < ScreenWidth && pixelPos.y < ScreenHeight)
    {
        float depth = DepthTexture.Load(int3(pixelPos, 0));

        // Convert depth [0,1] to view-space Z
        float viewZ = NearFar.x * NearFar.y / (NearFar.y - depth * (NearFar.y - NearFar.x));

        // Convert float to uint for atomic operations (preserve ordering)
        uint depthInt = asuint(viewZ);

        // Update shared min/max using atomics on uint
        InterlockedMin(TileMinDepthInt, depthInt);
        InterlockedMax(TileMaxDepthInt, depthInt);
    }

    GroupMemoryBarrierWithGroupSync();

    // Convert back to float
    float TileMinDepth = asfloat(TileMinDepthInt);
    float TileMaxDepth = asfloat(TileMaxDepthInt);

    // Step 2: Calculate tile bounds in view space
    // Tile boundaries in screen space [0, ScreenWidth/Height]
    uint2 tileMin = groupID.xy * TILE_SIZE;
    uint2 tileMax = min(tileMin + uint2(TILE_SIZE, TILE_SIZE), uint2(ScreenWidth, ScreenHeight));

    // Convert screen coordinates to view space coordinates
    // In view space: origin at camera, X right, Y up, Z forward (into screen, negative)
    // Screen space: [0, ScreenWidth] x [0, ScreenHeight]
    // NDC: [-1, 1] x [-1, 1]

    // Simple approximation: use screen-space bounds as view-space X/Y bounds
    // For proper implementation, you'd construct frustum from screen corners
    float2 tileMinNorm = float2(tileMin) / float2(ScreenWidth, ScreenHeight) * 2.0 - 1.0;
    float2 tileMaxNorm = float2(tileMax) / float2(ScreenWidth, ScreenHeight) * 2.0 - 1.0;

    // Scale by depth to get view-space coordinates (approximation)
    float avgDepth = (TileMinDepth + TileMaxDepth) * 0.5f;
    float2 tileMinView = tileMinNorm * avgDepth;
    float2 tileMaxView = tileMaxNorm * avgDepth;

    // Step 3: Test lights against tile frustum
    uint totalLights = (uint)PointLightCount + (uint)SpotLightCount;
    uint numThreads = TILE_SIZE * TILE_SIZE;

    // Each thread processes multiple lights
    for (uint lightIndex = groupIndex; lightIndex < totalLights; lightIndex += numThreads)
    {
        bool intersects = false;
        float3 lightPosWorld;
        float lightRadius;

        // Get light data
        if (lightIndex < (uint)PointLightCount)
        {
            // Point light
            lightPosWorld = PointLights[lightIndex].Position.xyz;
            lightRadius = PointLights[lightIndex].Position.w;
        }
        else
        {
            // Spot light
            uint spotIndex = lightIndex - (uint)PointLightCount;
            lightPosWorld = SpotLights[spotIndex].Position.xyz;
            lightRadius = SpotLights[spotIndex].Position.w;
        }

        // Transform light position to view space
        float4 lightPosView4 = mul(float4(lightPosWorld, 1.0), ViewMatrix);
        float3 lightPosView = lightPosView4.xyz;

        // Test intersection with tile AABB
        intersects = SphereIntersectsTileAABB(
            lightPosView,
            lightRadius,
            tileMinView,
            tileMaxView,
            TileMinDepth,
            TileMaxDepth
        );

        // If intersects, add to shared list
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

    // Step 4: Write results to global memory (only first thread)
    if (groupIndex == 0)
    {
        uint tileIndex = groupID.y * NumTilesX + groupID.x;
        uint lightCount = min(TileLightCount, MAX_LIGHTS_PER_TILE);

        // Allocate space in global light index list
        uint offset;
        InterlockedAdd(LightIndexList[0], lightCount, offset);

        // Store offset and count for this tile
        TileOffsetCount[tileIndex] = uint2(offset + 1, lightCount);

        // Copy light indices to global buffer
        for (uint i = 0; i < lightCount; ++i)
        {
            LightIndexList[offset + 1 + i] = TileLightIndices[i];
        }
    }
}
