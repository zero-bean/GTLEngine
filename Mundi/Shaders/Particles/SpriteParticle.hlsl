cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

struct FMaterial
{
    float3 DiffuseColor; // Kd - Diffuse 색상
    float OpticalDensity; // Ni - 광학 밀도 (굴절률)
    float3 AmbientColor; // Ka - Ambient 색상
    float Transparency; // Tr or d - 투명도 (0=불투명, 1=투명)
    float3 SpecularColor; // Ks - Specular 색상
    float SpecularExponent; // Ns - Specular 지수 (광택도)
    float3 EmissiveColor; // Ke - 자체발광 색상
    uint IlluminationModel; // illum - 조명 모델
    float3 TransmissionFilter; // Tf - 투과 필터 색상
    float Padding; // 정렬을 위한 패딩
};

cbuffer PixelConstBuffer : register(b4)
{
    FMaterial Material; // 64 bytes
    uint bHasMaterial; // 4 bytes (HLSL)
    uint bHasTexture; // 4 bytes (HLSL)
    uint bHasNormalTexture;
};

struct VS_INPUT
{
    #ifdef MESH_PARTICLE
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT0;
    float4 MeshColor : MeshCOLOR;
    #else
    // Slot 0 : Quad (Per - Vertex)
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    #endif
    // Slot 1 : Instance (Per - Instance)
    float4 Color : COLOR;
    float3 WorldPos : TEXCOORD1;
    float3 Size : TEXCOORD2;
    float LifeTime : TEXCOORD3;
    float4 Rotation : TEXCOORD4;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
    float LifeTime : TEXCOORD1;
};


float3 RotateVectorByQuaternion(float3 v, float4 q)
{
    float3 qVec = q.xyz;
    float3 t = 2.0f * cross(qVec, v);
    return v + (q.w * t) + cross(qVec, t);
}


Texture2D SpriteTex : register(t0);
SamplerState LinearSampler : register(s0);

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    #ifdef MESH_PARTICLE

    Input.Rotation = normalize(Input.Rotation);
    float3 FinalPos = Input.Position * Input.Size;
    FinalPos = RotateVectorByQuaternion(FinalPos, Input.Rotation);
    FinalPos = FinalPos + Input.WorldPos;
    #else
    float3 CameraRight = float3(InverseViewMatrix._11, InverseViewMatrix._12, InverseViewMatrix._13);
    float3 CameraUp = float3(InverseViewMatrix._21, InverseViewMatrix._22, InverseViewMatrix._23);
    
    Input.Position = RotateVectorByQuaternion(Input.Position, Input.Rotation);
    //PositionRotated.x = Input.Position.x * cos(Input.Rotation.x) - Input.Position.y * sin(Input.Rotation.x);
    //PositionRotated.y = Input.Position.x * sin(Input.Rotation.x) + Input.Position.y * cos(Input.Rotation.x);
    float3 FinalPos = Input.WorldPos + (CameraRight * Input.Position.x * Input.Size.x) + (CameraUp * Input.Position.y * Input.Size.x);

    #endif
    float4 ViewPos = mul(float4(FinalPos, 1.0), ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.Color = Input.Color;
    Output.LifeTime = Input.LifeTime;
    Output.TexCoord = Input.TexCoord;
    
   

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float4 texColor = SpriteTex.Sample(LinearSampler, Input.TexCoord);
    float4 finalPixel = float4(0, 0, 0, 1);
    // 머티리얼/텍스처 블렌딩 적용
    if (bHasMaterial)
    {
        finalPixel.rgb = Material.DiffuseColor;
        if (bHasTexture)
        {
            finalPixel.rgb = texColor.rgb;
        }
        // 자체발광 추가
        finalPixel.rgb += Material.EmissiveColor;
    }
    else
    {
        // LerpColor와 블렌드
        //finalPixel.rgb = lerp(finalPixel.rgb, LerpColor.rgb, LerpColor.a);
        finalPixel.rgb *= texColor.rgb;
    }

    // 머티리얼 투명도 적용 (0=불투명, 1=투명)
    if (bHasMaterial)
    {
        finalPixel.a *= (1.0f - Material.Transparency);
    }
    
    return finalPixel;
}