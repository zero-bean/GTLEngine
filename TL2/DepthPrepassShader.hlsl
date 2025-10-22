// DepthPrepassShader.hlsl - Simple depth-only rendering
// Matches UberLit.hlsl structure but only outputs depth

cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    uint UUID;
    float3 Padding;
    row_major float4x4 NormalMatrix;
};

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    float3 CameraWorldPos; // 월드 기준 카메라 위치
    float _pad_cam; // 16바이트 정렬
};

// Match FVertexDynamic layout (same as UberLit.hlsl)
struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;

    // Transform: Local -> World -> View -> Projection (row_major)
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    output.position = mul(float4(input.position, 1.0f), MVP);

    return output;
}

// Depth-only: no pixel shader output needed
void mainPS()
{
    // Depth is automatically written to depth buffer
    // No color output required
}
