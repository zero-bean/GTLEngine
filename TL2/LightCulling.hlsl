// LightCulling.hlsl - Tile-based Light Culling Compute Shader

// Tile configuration
#define TILE_SIZE 32
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
// Projection inverse for depth reconstruction
cbuffer ViewMatrixCB : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjInv;
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

// Simple 2D circle-rectangle intersection (XY only, ignore depth)
bool SphereIntersectsTileAABB(float3 lightPosView, float lightRadius,
                              float2 tileMin, float2 tileMax,
                              float minDepth, float maxDepth)
{
    // Ignore depth, only check XY in view space
    // This is conservative but simple

    // Find closest point on 2D rectangle to light XY position
    float2 closest;
    closest.x = clamp(lightPosView.x, tileMin.x, tileMax.x);
    closest.y = clamp(lightPosView.y, tileMin.y, tileMax.y);

    // Check 2D distance
    float2 diff = lightPosView.xy - closest;
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
        TileMinDepthInt = asuint(10000.0f); // Far away
        TileMaxDepthInt = asuint(-10000.0f); // Close (negative in view space)
    }
    GroupMemoryBarrierWithGroupSync();

    // Calculate screen-space pixel coordinates
    uint2 pixelPos = groupID.xy * TILE_SIZE + groupThreadID.xy;

    // Step 1: Compute min/max depth for this tile
    if (pixelPos.x < ScreenWidth && pixelPos.y < ScreenHeight)
    {
        float depth = DepthTexture.Load(int3(pixelPos, 0));

        // Convert pixel position to NDC
        float2 ndc;
        ndc.x = (pixelPos.x / (float)ScreenWidth) * 2.0 - 1.0;
        ndc.y = -((pixelPos.y / (float)ScreenHeight) * 2.0 - 1.0); // Flip Y for D3D

        // Reconstruct view-space position using Inverse Projection
        float4 clipPos = float4(ndc, depth, 1.0);
        float4 viewPos = mul(clipPos, ProjInv);
        viewPos /= viewPos.w; // Perspective divide

        float viewZ = viewPos.z; // View-space Z (negative in right-handed view space)

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

    // DEBUG: Print depth and light info for first tile
    if (groupIndex == 0 && groupID.x == 0 && groupID.y == 0)
    {
        printf("Tile[0,0]: PointLights=%d, SpotLights=%d, MinDepth=%f, MaxDepth=%f\n",
               PointLightCount, SpotLightCount, TileMinDepth, TileMaxDepth);
    }

    // Step 2: Calculate tile bounds in view space (XY only, simplified)
    // Tile boundaries in screen space [0, ScreenWidth/Height]
    uint2 tileMin = groupID.xy * TILE_SIZE;
    uint2 tileMax = min(tileMin + uint2(TILE_SIZE, TILE_SIZE), uint2(ScreenWidth, ScreenHeight));

    // Simple approach: unproject corners at a fixed reference depth
    // Then we have the actual view-space XY range for this tile
    float2 viewMin = float2(100000.0, 100000.0);
    float2 viewMax = float2(-100000.0, -100000.0);

    // Use a fixed reference depth (e.g., -10 in view space)
    // The XY range scales linearly with depth in perspective projection
    float referenceDepth = -10.0f;

    // Check all 4 corners of the tile
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            // Get corner pixel position
            float2 pixelPos;
            pixelPos.x = (x == 0) ? (float)tileMin.x : (float)tileMax.x;
            pixelPos.y = (y == 0) ? (float)tileMin.y : (float)tileMax.y;

            // Convert to NDC
            float2 ndc;
            ndc.x = (pixelPos.x / (float)ScreenWidth) * 2.0 - 1.0;
            ndc.y = -((pixelPos.y / (float)ScreenHeight) * 2.0 - 1.0);

            // Unproject: viewXY = ndcXY * viewZ * [projection factors]
            // Use ProjInv to get projection factors
            float4 clipPos = float4(ndc.x, ndc.y, 0.0, 1.0);
            float4 viewPos = mul(clipPos, ProjInv);
            viewPos.xyz /= viewPos.w;

            // Scale to reference depth
            float2 scaledXY = viewPos.xy * (referenceDepth / viewPos.z);

            viewMin = min(viewMin, scaledXY);
            viewMax = max(viewMax, scaledXY);
        }
    }

    float2 tileMinView = viewMin;
    float2 tileMaxView = viewMax;

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

        // Transform light position to view space, then to screen space
        float4 lightPosView4 = mul(float4(lightPosWorld, 1.0), ViewMatrix);
        float3 lightPosView = lightPosView4.xyz;

        // Project light to screen space (NDC, then screen pixels)
        float4 lightPosClip = mul(lightPosView4, transpose(ProjInv)); // Need actual Proj matrix, not ProjInv!
        // PROBLEM: We have ProjInv, not Proj. Let's use a different approach.

        // Alternative: Check if light XY is within tile's screen bounds
        // Project light view position to NDC
        // We need Proj matrix, but we only have ProjInv

        // For now: Simple screen-space check using tile pixel bounds
        // Convert tile bounds to NDC, check if light projects into that range

        // Tile screen bounds
        float2 tileScreenMin = float2(tileMin.x, tileMin.y);
        float2 tileScreenMax = float2(tileMax.x, tileMax.y);

        // Convert to NDC
        float2 tileNDCMin = (tileScreenMin / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
        float2 tileNDCMax = (tileScreenMax / float2(ScreenWidth, ScreenHeight)) * 2.0 - 1.0;
        tileNDCMin.y = -tileNDCMin.y;
        tileNDCMax.y = -tileNDCMax.y;

        // Correct min/max after Y flip
        float2 ndcMin = float2(min(tileNDCMin.x, tileNDCMax.x), min(tileNDCMin.y, tileNDCMax.y));
        float2 ndcMax = float2(max(tileNDCMin.x, tileNDCMax.x), max(tileNDCMin.y, tileNDCMax.y));

        // Project light to NDC (we need to reconstruct Proj from ProjInv, or use formula)
        // Standard perspective: ndcX = viewX * Proj[0][0] / viewZ
        // Extract Proj[0][0] and Proj[1][1] from ProjInv
        // ProjInv[0][0] = 1 / Proj[0][0]

        float projScale = 1.0 / ProjInv[0][0];
        float2 lightNDC;
        // Note: viewZ is negative, so we use -viewZ to make it positive
        // X: Flip to match screen coordinates
        lightNDC.x = -(lightPosView.x * projScale / (-lightPosView.z));
        // Y: Flip Y
        lightNDC.y = -(lightPosView.y * (1.0 / ProjInv[1][1]) / (-lightPosView.z));

        // Check if light NDC is within tile NDC bounds (with radius margin)
        // Convert radius to NDC space
        // radiusNDC should be in screen-space proportion
        float radiusNDC = lightRadius * projScale / abs(lightPosView.z);

        // Use actual radius (uncomment when direction is correct)
        // radiusNDC = 0.0; // DEBUG: point test only

        // 2D circle-rectangle test in NDC space
        float2 closest;
        closest.x = clamp(lightNDC.x, ndcMin.x, ndcMax.x);
        closest.y = clamp(lightNDC.y, ndcMin.y, ndcMax.y);

        float2 diff = lightNDC - closest;
        float distSq = dot(diff, diff);

        intersects = (distSq <= radiusNDC * radiusNDC);

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
        for (uint i = 0; i < 10; ++i)
        {
            LightIndexList[offset + 1 + i] = TileLightIndices[i];
        }
    }
}
