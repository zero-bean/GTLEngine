// ShaderW0.hlsl

cbuffer constants : register(b0)
{
    float4 color;
    float3 WorldPosition;
    float Scale;
    float4x4 roatation;
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
    //float4 scaledPosition = { input.position.x * Scale,input.position.y * Scale,input.position.z * Scale, 1 };

    // 상수버퍼를 통해 넘겨 받은 WorldPosition을 더해서 모든 정점을 
    //scaledPosition = mul(roatation, scaledPosition);
    //output.position = float4(WorldPosition, 0) + scaledPosition;
    //output.color = input.color;
    
    float4 pos = input.position * Scale;
    pos = mul(roatation, float4(pos.xyz, 1.0f));
    float4 pos2 = float4(pos.xy, 0.0, 1.0);
    output.position = float4(WorldPosition, 0) + pos2;
    
    //output.position = input.position;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return input.color;
}