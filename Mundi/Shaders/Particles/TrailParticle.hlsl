cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

struct VS_INPUT
{
    float4 Color : COLOR;
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    float4 ViewPos = mul(float4(Input.Position, 1.0f), ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);

    Output.Color = Input.Color;
    Output.TexCoord = Input.TexCoord;

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    return Input.Color;
}