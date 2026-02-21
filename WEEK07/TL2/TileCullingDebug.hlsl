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
// ★★★ 1. 화면 절대 좌표에서 뷰포트 오프셋을 빼서 "뷰포트 상대 좌표"를 구함 ★★★
    // (input.position.xy는 SV_POSITION에 의해 픽셀의 화면 절대 좌표가 됨)
    float2 viewportPixelPos = input.position.xy - ViewportOffset; // ViewportOffset은 Light.hlsli에서 옴

    // ★★★ 2. 뷰포트 영역 밖의 픽셀은 그리지 않음 (discard) ★★★
    if (viewportPixelPos.x < 0 || viewportPixelPos.y < 0)
    {
        discard;
    }

    // ★★★ 3. "뷰포트 상대 좌표"를 기준으로 모든 계산 수행 ★★★
    uint2 pixelPos = (uint2) viewportPixelPos;
    uint2 tileID = pixelPos / TILE_SIZE;
    uint2 pixelInTile = pixelPos % TILE_SIZE;

    // (선택 사항) 뷰포트 크기 밖의 타일 ID가 계산될 경우(예: 뷰포트 크기가 32배수가 아닐 때)
    if (tileID.x >= NumTilesX) // NumTilesX는 Light.hlsli에서 옴
    {
        discard;
    }
    
    // ★★★ 4. 올바른 타일 인덱스 계산 ★★★
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
        // return float4(1.0, 0.0, 0.0, 0.1);
    }

    // Show actual light count as heat map
    float3 heatColor = LightCountToHeatMap(lightCount) * 1.5;

    return float4(heatColor, 0.3); // More transparent to see scene underneath
}
