//================================================================================================
// Filename:      ShadowVSM_VS.hlsl
// Description:   VSM (Variance Shadow Maps) 섀도우 맵 생성용 버텍스 쉐이더
//================================================================================================

// b0: Model Matrix
cbuffer ModelBuffer : register(b0)
{
    row_major matrix Model;
    row_major matrix ModelInverseTranspose;
};

// b1: View-Projection Matrix (라이트 기준)
cbuffer ViewProjBuffer : register(b1)
{
    row_major matrix View;
    row_major matrix Proj;
    row_major matrix InvView;
    row_major matrix InvProj;
};

struct VS_INPUT
{
    float3 Position : POSITION;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;  // 화면 공간 위치
    float Depth : TEXCOORD0;        // 라이트 공간 depth (0~1)
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;

    // World space 변환
    float4 worldPos = mul(float4(input.Position, 1.0f), Model);

    // Light space 변환
    float4 viewPos = mul(worldPos, View);
    float4 projPos = mul(viewPos, Proj);

    output.Position = projPos;

    // Depth 계산 (0~1 범위로 정규화)
    output.Depth = projPos.z / projPos.w;

    return output;
}
