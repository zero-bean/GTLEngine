
Texture2D FrameColor : register(t0);
SamplerState LinearSampler : register(s0);


//float SlideX = 1.0f;
//float SpanMax = 8.0f; 
//float ReduceMin = 1.0f / 128.0f;
//float ReduceMul = 1.0f / 8.0f;
cbuffer FXAACBuffer : register(b0)
{
    float SlideX;
    float SpanMax;
    float ReduceMin;
    float ReduceMul;
}

struct PS_Input
{
    float4 posCS : SV_Position;
    float2 uv : TEXCOORD0;
};

PS_Input mainVS(uint Input : SV_VertexID)
{
     PS_Input o;
   
    float2 UVMap[] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
    };

    o.uv = UVMap[Input];
    o.posCS = float4(o.uv.x * 2.0f - 1.0f, 1.0f - (o.uv.y * 2.0f), 0.0f, 1.0f);
    return o;
}


//Get Texture Info
//uint OutTexWidth, OutTexHeight, OutMipCount = 0;
//uint InMipLevel = 0;
//FrameColor.GetDimensions(InMipLevel, TexWidth, TexHeight, MipCount);

float GetFrameSample(float2 uv)
{
    float3 Lumaniance = float3(0.299f, 0.587f, 0.114f);
    return dot(FrameColor.Sample(LinearSampler, uv).rgb, Lumaniance);
}

float4 mainPS(PS_Input i) : SV_TARGET
{
    if (i.uv.x > SlideX)
    {
        discard;
    }
    if (!(SlideX == 0.00f || SlideX == 1.0f))
    {
        if (0.005f > abs(i.uv.x - SlideX))
        {
            return float4(1, 0, 0, 1);
        }
    }
    float3 Lumaniance = float3(0.299f, 0.587f, 0.114f);
    
    //Get Texture Info
    uint TexWidth, TexHeight, MipCount = 0;
    FrameColor.GetDimensions(0, TexWidth, TexHeight, MipCount);
    float2 TexSizeRCP = float2(1.0f / TexWidth, 1.0f / TexHeight);
    float2 uv = float2(i.posCS.x / TexWidth, i.posCS.y / TexHeight);
    
    
    float3 Color = FrameColor.Sample(LinearSampler, uv).rgb;
    //NearPixel Sample
    float M = dot(Color, Lumaniance);
    float TR = GetFrameSample(uv + int2(1, 1) * TexSizeRCP);
    float TL = GetFrameSample(uv + int2(-1, 1) * TexSizeRCP);
    float BR = GetFrameSample(uv + int2(1, -1) * TexSizeRCP);
    float BL = GetFrameSample(uv + int2(-1, -1) * TexSizeRCP);
    

    float2 Dir; //밝기 차이 
    Dir.x = -((TR + TL) - (BR + BL)); //Top - Bottom  음수 => 위가 밝다, 양수 => 아래가 밝다
    Dir.y = ((TR + BR) - (TL + BL)); //Right - Left 양수 => 오른쪽이 밝다, 음수 => 왼쪽이 밝다
	
    float DirReduce = max((TR + TL + BR + BL) * 0.25 * ReduceMul, ReduceMin); //DirReduce = max(대각 4점 평균 * ReduceMul, ReduceMin) = max(대각 평균 줄인거, ReduceMin)
    float InvDirAdjustment = 1.0 / (min(abs(Dir.x), abs(Dir.y)) + DirReduce); //1 / (밝기차이절대값X,Y중 최소 + DirReduce) //dir 작은값이 0일 수 있어서 최소값을 더함
	
    //Dir * InvDirAdjustment = Dir / (밝기차이절대값X,Y중 최소 + max(대각평균 줄인거, ReduceMin))
    Dir = min(float2(SpanMax, SpanMax), max(float2(-SpanMax, -SpanMax), Dir * InvDirAdjustment)); //Dir = -SpanMax <= (Dir * InvDirAdjustment) <= SpanMax 범위로 넣기

    //가로세로 확인
    //코드

    //if (abs(Dir.x) < 1 && abs(Dir.y) < 1)
    //{
        
    //}
    //else if (abs(Dir.x) < 1)
    //{
    //    return float4(1, 1, 0, 1);
    //}
    //else if (abs(Dir.y) < 1)
    //{
    //    return float4(0, 1, 1, 1);
    //}
    
    //step(x,y) = x <= y ? 1 : 0;
    Dir.x = Dir.x * step(1.0, abs(Dir.x)); //abs(Dir.x)가 1보다 크거나 같으면 1 else 0
    Dir.y = Dir.y * step(1.0, abs(Dir.y));
    Dir *= TexSizeRCP; //1 / TexSize 곱함
    

    //(0, 1/3, 2/3, 1) => (-0.5, (1/3 - 0.5), (2/3 - 0.5), 0.5) 
    float3 Sample1 = FrameColor.Sample(LinearSampler, uv + (Dir * (1.0f / 3.0f - 0.5f)));
    float3 Sample2 = FrameColor.Sample(LinearSampler, uv + (Dir * (2.0f / 3.0f - 0.5f)));
    float3 Sample3 = FrameColor.Sample(LinearSampler, uv + (Dir * (0.0f / 3.0f - 0.5f)));
    float3 Sample4 = FrameColor.Sample(LinearSampler, uv + (Dir * (3.0f / 3.0f - 0.5f)));

    float3 Result1 = 0.5f * (Sample1 + Sample2); //가까운 두점 평균
    float3 Result2 = 0.25f * (Sample1 + Sample2 + Sample3 + Sample4); //4점 평균

    float Min = min(M, min(min(TL, TR), min(BL, BR)));
    float Max = max(M, max(max(TL, TR), max(BL, BR)));
    float Result2Dot = dot(Lumaniance, Result2);
	
    //4점평균이 Min Max 범위 밖이면 Result1(가까움 두점 평균) 리턴
    if (Result2Dot < Min || Result2Dot > Max)
    {
        return float4(Result1, 1.0);
    }
    else
    {
        return float4(Result2, 1.0);
    }
}
		





//fail
//float4 mainPS(PS_Input i) : SV_TARGET
//{
//    float3 Lumaniance = float3(0.299f, 0.587f, 0.114f);
//    float ContrastThreshold = 0.0312f;
//    float RelativeThreshold = 0.063f;
//    float FXAAStrength = 1.0f;
    
//    //Get Texture Info
//    uint TexWidth, TexHeight, MipCount = 0;
//    FrameColor.GetDimensions(0, TexWidth, TexHeight, MipCount);
//    float2 TexSizeRCP = float2(1.0f / TexWidth, 1.0f / TexHeight);
//    float2 uv = float2(i.posCS.x / TexWidth, i. posCS.y / TexHeight);
    
//    float3 Color = FrameColor.Sample(LinearSampler, uv).rgb;
//    //NearPixel Sample
//    float M = dot(Color, Lumaniance);
//    float N = GetFrameSample(uv + int2( 0,  1) * TexSizeRCP);
//    float S = GetFrameSample(uv + int2( 0, -1) * TexSizeRCP);
//    float E = GetFrameSample(uv + int2( 1,  0) * TexSizeRCP);
//    float W = GetFrameSample(uv + int2(-1,  0) * TexSizeRCP);
    
//    //Calc Contrast
//    float Lowest = min(M, min(N, min(S, min(E, W))));
//    float Heighest = max(M, max(N, max(S, max(E, W))));
//    float Contrast = Heighest - Lowest;
    
//    //Calc Threshold
//    float Threshold = max(ContrastThreshold, RelativeThreshold * Heighest);
//    if(Contrast < Threshold)
//    {
//        //return float4(1, 1, 1, 1);
//        discard;
//    }
    
    
//    //1 2 1
//    //2 M 2
//    //1 2 1
//    float NE = GetFrameSample(uv + int2( 1,  1) * TexSizeRCP);
//    float NW = GetFrameSample(uv + int2(-1,  1) * TexSizeRCP);
//    float SE = GetFrameSample(uv + int2( 1, -1) * TexSizeRCP);
//    float SW = GetFrameSample(uv + int2(-1, -1) * TexSizeRCP);
    
//    float filter = (2 * (N + S + E + W) + (NE + NW + SE + SW)) / 12.0f;
//    float BlendFactor = saturate(abs(filter - M) / Contrast);
//    BlendFactor = smoothstep(0, 1, BlendFactor);
//    BlendFactor = BlendFactor * BlendFactor;
//    float Horizontal = 2 * abs(N + S - 2 * M) + abs(NE + SE - 2 * E) + abs(NW + SW - 2 * W); 
//    float Vertical = 2 * abs(E + W - 2 * M) + abs(NE + NW - 2 * N) + abs(SE + SW - 2 * S);
//    bool bHorizontal = Horizontal > Vertical;
    
//    float2 tempColor = bHorizontal ? float2(1, 0) : float2(0, 1);
//    //return float4(tempColor, Color.b, 1); //가로 r 세로 g 로 출력

    
//    float PLuminance = bHorizontal ? N : E; 
//    float NLuminance = bHorizontal ? S : W; 
//    float PGradient = abs(PLuminance - M);
//    float NGradient = abs(NLuminance - M);
//    float Gradient = PGradient > NGradient ? PGradient : NGradient;
    
//    //엣지 바깥으로 향하도록 하는 uv 스텝량 (가로선에 위가 바깥일 경우 => (0, TexSizeRCP.y)
//    float2 ToEdgeSideUVStep = bHorizontal ? float2(0, TexSizeRCP.y) : float2(TexSizeRCP.x, 0);
//    ToEdgeSideUVStep = PGradient > NGradient ? ToEdgeSideUVStep : -ToEdgeSideUVStep;
    
//    //엣지 기울기 검출
//    float2 EdgeStep = bHorizontal ? float2(TexSizeRCP.x, 0) : float2(0, TexSizeRCP.y);
//    float2 CurUV = uv + ToEdgeSideUVStep * 0.5f;
//    float EdgeSideLuminanceDot = GetFrameSample(CurUV);

    
    
//    float GradientThreshold = abs(Gradient * 0.25f);
//    float StepDis = max(abs(EdgeStep.x), abs(EdgeStep.y));

//    //엣지 검출에 Luminance dot값을 사용하는데, 컬러는 다르고 Luminance dot은 같다면 엣지검출이 안될텐데?
//    float2 PCurUV = CurUV;
//    float PDistance = 0;
//    float PLuminanceDelta = 0;
//    bool bPAtEnd = 0;


//    for (int i = 0; i < 30 && bPAtEnd == false; i++)
//    {
//        PCurUV += EdgeStep;
//        PLuminanceDelta = GetFrameSample(PDistance) - EdgeSideLuminanceDot;
//        PDistance += StepDis;
//        bPAtEnd = abs(PLuminanceDelta) > GradientThreshold;
//    }
    
//    float2 NCurUV = CurUV;
//    float NDistance = 0;
//    float NLuminanceDelta = 0;
//    bool bNAtEnd = 0;
    
//    for (int j = 0; j < 30 && bNAtEnd == false; j++)
//    {
//        NCurUV -= EdgeStep;
//        NLuminanceDelta = GetFrameSample(NCurUV) - EdgeSideLuminanceDot;
//        NDistance += StepDis;
//        bNAtEnd = abs(NLuminanceDelta) > GradientThreshold;
//    }

//    float ShortestDis = PDistance < NDistance ? PDistance : NDistance;
//    ShortestDis *= 10;
//    float MaxBlending = max(ShortestDis, BlendFactor);
//    if(ShortestDis > BlendFactor)
//    {
//        return float4(ShortestDis, 0, 0, 1);
//    }
//    else
//    {
//        return float4(0, BlendFactor, 0, 1);

//    }
//    uv += ToEdgeSideUVStep * MaxBlending;
//    float4 Result = float4(FrameColor.Sample(LinearSampler, uv).rgb, 1);
    
//      return Result;
//}

