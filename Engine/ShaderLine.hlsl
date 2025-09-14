cbuffer Constants : register(b0)
{
    float4x4 MVP;
    float4 LineColor;
}
struct VSInput
{
    float3 position : POSITION; // Vertex position (x, y, z)
};
struct VSOutput
{
    float4 position : SV_POSITION; // Vertex position (x, y, z, w)
};
VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), MVP);
   
    return output;
}
struct PSInput
{
    float4 position : SV_Position;
};
float4 main(PSInput input) : SV_TARGET
{
    return LineColor;
}