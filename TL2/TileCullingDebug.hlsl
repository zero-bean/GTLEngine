// TileCullingDebug.hlsl - Full-screen debug visualization for tile-based light culling
#include "Light.hlsli"

struct VS_INPUT
{
    uint vertexID : SV_VertexID;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// Full-screen triangle vertex shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // Generate full-screen triangle using vertex ID
    // Vertex 0: (-1, -1) -> (0, 1)
    // Vertex 1: (-1,  3) -> (0, -1)
    // Vertex 2: ( 3, -1) -> (2, 1)
    float2 texCoord = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.position = float4(texCoord * float2(2, -2) + float2(-1, 1), 0, 1);
    output.texCoord = texCoord;

    return output;
}

// Pixel shader: visualize tile light count as heat map + tile grid lines
float4 mainPS(PS_INPUT input) : SV_Target
{
    // Calculate tile ID and position within tile
    uint2 pixelPos = uint2(input.position.xy);
    uint2 tileID = pixelPos / TILE_SIZE;
    uint2 pixelInTile = pixelPos % TILE_SIZE;
    uint tileIndex = tileID.y * NumTilesX + tileID.x;

    // Draw tile border lines (1 pixel thick)
    bool isBorder = (pixelInTile.x == 0 || pixelInTile.y == 0);

    // Try to read the offset/count
    uint2 offsetCount = g_TileOffsetCount[tileIndex];
    uint lightCount = offsetCount.y;

    // Draw tile grid lines (soft gray, semi-transparent)
    if (isBorder)
    {
        return float4(0.5, 0.5, 0.5, 0.5); // Soft gray border
    }

    // If no data in buffer, show test pattern based on tile position
    if (lightCount == 0 && offsetCount.x == 0)
    {
        // Show very transparent red to indicate buffer might be empty
        return float4(1.0, 0.0, 0.0, 0.1);
    }

    // Show actual light count as heat map
    float3 heatColor = LightCountToHeatMap(lightCount);
    return float4(heatColor, 0.3); // More transparent to see scene underneath
}
