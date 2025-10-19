#pragma once

inline FMatrix XYZtoRGB = {
	3.2406f, -1.5372f, -0.4986f, 0.0f,
	-0.9689f, 1.8758f, 0.0415f, 0.0f,
	0.0557f, -0.2040f, 1.0570f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

struct FLinearColor
{
	float R;
	float G;
	float B;
	float A;
};

inline FLinearColor MakeFromColorTemperature(float InTemperature)
{
	float Temperature = std::clamp(InTemperature, 1000.0f, 15000.0f);
    
	// CIE 1960 UCS에서의 Plakian locus
	// CIE 1960 UCS 색공간에서 흑체의 온도에 따른 이동 경로를 나타내는 식
	float U = (0.86011777f + (1.54118254e-4f * Temperature) + (1.28641212e-7f * Temperature * Temperature)) /
			(1.0f + (8.42420235e-4f * Temperature) + (7.08145163e-7 * Temperature * Temperature));
	float V = (0.317398726f + (4.22806245e-5f * Temperature) + (4.20481693e-8f * Temperature * Temperature)) /
			(1.0f - (2.89741816e-5f * Temperature) + (1.61456053e-7 * Temperature * Temperature));

	// CIE(x, y) 색도 좌표
	float x = (3.0f * U) / ((2.0f * U) - (8.0f * V) + 4);
	float y = (2.0f * U) / ((2.0f * U) - (8.0f * V) + 4);
	float z = 1.0f - x - y;

	// CIE XYZ 색공간
	// 밝기 Y = 1.0f로 설정
	// 나눗셈 최적화용으로 미리 계산
	float Invy = 1.0f / y;
	FVector4 XYZColorSpace = FVector4(Invy * x, 1.0f, Invy * z, 0.0f);
	FVector4 NewRGB = XYZtoRGB.TransformPosition(XYZColorSpace);    
	return FLinearColor(
		std::max(0.0f, NewRGB.X),
		std::max(0.0f, NewRGB.Y),
		std::max(0.0f, NewRGB.Z),
		1.0f);    
}

inline FLinearColor operator*(const FLinearColor& LHS, const FLinearColor& RHS)
{
	FLinearColor Result{};
	Result.R = LHS.R * RHS.R;
	Result.G = LHS.G * RHS.G;
	Result.B = LHS.B * RHS.B;
	Result.A = LHS.A * RHS.A;
	return Result;
}