cbuffer Constants : register(b0)
{
    row_major float4x4 MVP;
}

struct VSInput
{
    float4 position : POSITION; // Vertex position (x, y, z)
    float4 Color    : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION; // Vertex position (x, y, z, w)
    float4 Color : COLOR;
};

VSOutput VS_Main(VSInput input)
{
    VSOutput output;
    output.position = mul(input.position, MVP);
    output.Color = input.Color;
   
    return output;
}

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

float4 PS_Main(PSInput input) : SV_TARGET
{
    return input.color;
}