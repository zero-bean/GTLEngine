//======================================================
// DecalSpotLightShader.hlsl - Forward 렌더링으로 projection decal Shading
//======================================================

cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
}

cbuffer ColorBuffer : register(b3)
{
    float4 LerpColor;
}

cbuffer DecalTransformBuffer : register(b7)
{
    row_major float4x4 DecalWorldMatrix;
    row_major float4x4 DecalWorldMatrixInverse;
    row_major float4x4 DecalProjectionMatrix;
}

cbuffer DecalFXBuffer : register(b8)
{
    float CurrentAlpha;
}

//------------------------------------------------------
// Resources
//------------------------------------------------------
Texture2D g_DecalTexture : register(t0);
SamplerState g_Sample : register(s0);

//------------------------------------------------------
// Vertex Shader
//------------------------------------------------------
struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 픽셀 셰이더에서 자동으로 화면 픽셀 좌표로 변환됨
    float3 WorldPosition : WORLDPOSITION;
};

struct PS_OUTPUT
{
    float4 Color : SV_TARGET;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 월드 → 뷰 → 프로젝션 (row_major 기준)
    float4 WorldPosition = mul(float4(input.position, 1.0f), WorldMatrix);

    float4 ClipPosition = mul(mul(WorldPosition, ViewMatrix), ProjectionMatrix);
    output.position = ClipPosition;
    output.WorldPosition = WorldPosition.xyz;
    return output;
}


//------------------------------------------------------
// Pixel Shader
//------------------------------------------------------
PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;
    float4 DecalPosition = mul(float4(input.WorldPosition, 1.0f), DecalWorldMatrixInverse);
        
    // +-+-+ Perspective Projection +-+-+
    float4 ProjPos = mul(DecalPosition, DecalProjectionMatrix);
    ProjPos.xyz /= ProjPos.w;
    
    // SpotLight Frustum 내부 판정
    float r = sqrt(ProjPos.x * ProjPos.x + ProjPos.y * ProjPos.y);
    
    // ndc에서 원형으로 클리핑
    if (r > 1.0f || ProjPos.z < 0.0f || ProjPos.z > 1.0f)
    {
        discard;
    }
    // edge fade-out 효과 추가
    float edgeFade = saturate(1.0f - pow(r, 3.0f));
    
    // +-+-+ UV Calculation +-+-+
    float2 DecalUV = float2(
        ProjPos.x * 0.5f + 0.5f,
        1.0f - (ProjPos.y * 0.5f + 0.5f)
    );
    
    // +-+-+ Surface Normal, Fade Angle, etc +-+-+
    float3 dpdx = ddx(input.WorldPosition);
    float3 dpdy = ddy(input.WorldPosition);
    float3 surfaceNormal = -normalize(cross(dpdy, dpdx));

    // 표면이 데칼 중심을 향하는지 판단
    float3 spotPosition = DecalWorldMatrix[3].xyz;
    float3 toPixel = normalize(input.WorldPosition - spotPosition);
    float facing = dot(surfaceNormal, toPixel);
    if (facing > 0.0f)
    {
        discard;
    }
    
    // +-+-+ Edge Fade +-+-+
    //float2 uvFrac = frac(DecalUV); // 0~1 범위로 반복
    //float2 edgeDistance = abs(uvFrac - 0.5f) * 2.0f; // 0(중심) ~ 1(가장자리)
    //float edgeFade = saturate(1.0f - max(edgeDistance.x, edgeDistance.y));
    //edgeFade = pow(edgeFade, 0.05f); // 가장자리에서 부드럽게 페이드

    float4 DecalColor = g_DecalTexture.Sample(g_Sample, DecalUV);

    // 최종 알파 = 텍스처 알파 * 각도 페이드 * 가장자리 페이드
    DecalColor.a *= edgeFade;
    DecalColor.a *= CurrentAlpha;
    
    Result.Color = DecalColor;
    return Result;
}
