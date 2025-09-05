cbuffer constants : register(b0)
{
    float3 Offset;
    float ScaleX;
    float ScaleY;
    float Rotation;
    float Radius;
}

struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 1) 스케일 (a=ScaleX, b=ScaleY)
    float2 scaled = float2(input.position.x * ScaleX, input.position.y * ScaleY);

    // 2) 이등변 삼각형 인센터: Base(0)에서 위로 Radius 만큼
    // (다른 Primitive는 Radius=0으로 전달된다고 가정)
    scaled.y -= Radius;

    // 3) 회전
    float c = cos(Rotation);
    float s = sin(Rotation);
    float2 rotated = float2(scaled.x * c - scaled.y * s,
                            scaled.x * s + scaled.y * c);

    // 4) Offset 적용
    float2 world = rotated + Offset.xy;

    output.position = float4(world.xy, input.position.z + Offset.z, 1.0f);

    // 색상은 그대로 전달합니다.
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
