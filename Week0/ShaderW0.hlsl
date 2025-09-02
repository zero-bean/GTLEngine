// ShaderW0.hlsl

cbuffer constants : register(b0)
{
    float3 WorldPosition;
    float Scale;
}

struct VS_INPUT
{
    float4 position : POSITION; 
    float4 color : COLOR;       
};

struct PS_INPUT
{
    float4 position : SV_POSITION; 
    float4 color : COLOR;          
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    //구의 크기를 scale값으로 조정
    float4 scaledPosition = { input.position.x * Scale,input.position.y * Scale,input.position.z * Scale, 1 };

    // 상수버퍼를 통해 넘겨 받은 WorldPosition을 더해서 모든 정점을 
    output.position = float4(WorldPosition, 0) + scaledPosition;
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return input.color;
}