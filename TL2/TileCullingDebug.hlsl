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

// Pixel shader: visualize tile light count as heat map
float4 mainPS(PS_INPUT input) : SV_Target
{
    // DEBUG: Test if we can read tile data
    uint2 tileID = uint2(input.position.xy) / TILE_SIZE;
    uint tileIndex = tileID.y * NumTilesX + tileID.x;

    // Try to read the offset/count
    uint2 offsetCount = g_TileOffsetCount[tileIndex];
    uint lightCount = offsetCount.y;

    // If no data in buffer, show test pattern based on tile position
    if (lightCount == 0 && offsetCount.x == 0)
    {
        // Show red to indicate buffer might be empty
        return float4(1.0, 0.0, 0.0, 0.3);
    }

    // Show actual light count
    float3 heatColor = LightCountToHeatMap(lightCount);
    return float4(heatColor, 0.7);
}
