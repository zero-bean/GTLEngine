cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

struct VS_INPUT
{
    // Slot 0 : Quad (Per - Vertex)
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    // Slot 1 : Instance (Per - Instance)
    float4 Color : COLOR;
    float3 WorldPos : TEXCOORD1;
    float Size : TEXCOORD2;
    float LifeTime : TEXCOORD3;
    float Rotation : TEXCOORD4;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
    float LifeTime : TEXCOORD1;
};

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    float3 CameraRight = float3(ViewMatrix._11, ViewMatrix._12, ViewMatrix._13);
    float3 CameraUp = float3(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23);

    float3 FinalPos = Input.WorldPos + (CameraRight * Input.Position.x * Input.Size) + (CameraUp * Input.Position.y * Input.Size);

    float4 ViewPos = mul(float4(FinalPos, 1.0), ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.Color = Input.Color;
    Output.LifeTime = Input.LifeTime;
    Output.TexCoord = Input.TexCoord;

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    return Input.Color;
    // float2 Center = float2(0.5, 0.5);
    // float Distance = distance(Input.TexCoord, Center);
    //
    // if (Distance > 0.5)
    // {
    //     discard;
    // }
    //
    // // float Alpha = smoothstep(0.45, 0.5, Distance);
    // float Alpha = 1.0;
    //
    // // 점점 투명해짐
    // if (Input.LifeTime < 0.2)
    // {
    //     Alpha *= (Input.LifeTime / 0.2);
    // }
    //
    // return float4(Input.Color.rgb, Input.Color.a * Alpha);
}