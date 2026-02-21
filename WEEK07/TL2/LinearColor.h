#pragma once

inline FMatrix XYZtoRGB = {
	3.2404542f, -1.5371385f, -0.4985314f, 0.0f,
	-0.9692660f, 1.8760108f, 0.0415560f, 0.0f,
	0.0556434f, -0.2040259f, 1.0572252f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

struct FLinearColor
{
	float R;
	float G;
	float B;
	float A;
};

inline FVector4 ConvertXYZToRGB(const FVector4& V, FMatrix& Matrix)
{
	return FVector4(
		// Result.X = dot(V, Row 0)
		V.X * Matrix.M[0][0] + V.Y * Matrix.M[0][1] + V.Z * Matrix.M[0][2] + V.W * Matrix.M[0][3],
        
		// Result.Y = dot(V, Row 1)
		V.X * Matrix.M[1][0] + V.Y * Matrix.M[1][1] + V.Z * Matrix.M[1][2] + V.W * Matrix.M[1][3],
        
		// Result.Z = dot(V, Row 2)
		V.X * Matrix.M[2][0] + V.Y * Matrix.M[2][1] + V.Z * Matrix.M[2][2] + V.W * Matrix.M[2][3],
        
		// Result.W = dot(V, Row 3)
		V.X * Matrix.M[3][0] + V.Y * Matrix.M[3][1] + V.Z * Matrix.M[3][2] + V.W * Matrix.M[3][3]
	);
}

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
	float y = (2.0f * V) / ((2.0f * U) - (8.0f * V) + 4);
	float z = 1.0f - x - y;

	// CIE XYZ 색공간
	// 밝기 Y = 1.0f로 설정
	// 나눗셈 최적화용으로 미리 계산
	float Invy = 1.0f / y;
	FVector4 XYZColorSpace = FVector4(Invy * x, 1.0f, Invy * z, 0.0f);
	FVector4 NewRGB = ConvertXYZToRGB(XYZColorSpace, XYZtoRGB);
	
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

inline FLinearColor operator*(float LHS, const FLinearColor& RHS)
{
	FLinearColor Result{};
	Result.R = LHS * RHS.R;
	Result.G = LHS * RHS.G;
	Result.B = LHS * RHS.B;
	Result.A = LHS * RHS.A;
	return Result;
}

inline FLinearColor operator*(const FLinearColor& LHS, float RHS)
{
	FLinearColor Result{};
	Result.R = LHS.R * RHS;
	Result.G = LHS.G * RHS;
	Result.B = LHS.B * RHS;
	Result.A = LHS.A * RHS;
	return Result;
}

inline bool operator==(const FLinearColor& LHS, const FLinearColor& RHS)
{
	return (LHS.R == RHS.R) && (LHS.G == RHS.G) && (LHS.B == RHS.B) && (LHS.A == RHS.A);
}