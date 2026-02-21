#include "pch.h"
#include "Vector.h"

#include "Core/Public/Archive.h"

/**
 * @brief FVector 기본 생성자
 */
FVector::FVector()
	: X(0), Y(0), Z(0)
{
}


/**
 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
 */
FVector::FVector(float InX, float InY, float InZ)
	: X(InX), Y(InY), Z(InZ)
{
}


/**
 * @brief FVector를 Param으로 넘기는 생성자
 */
FVector::FVector(const FVector& InOther)
	: X(InOther.X), Y(InOther.Y), Z(InOther.Z)
{
}

void FVector::operator=(const FVector4& InOther)
{
	*this = FVector(InOther.X, InOther.Y, InOther.Z);
}


/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector FVector::operator+(const FVector& InOther) const
{
	return { X + InOther.X, Y + InOther.Y, Z + InOther.Z };
}

/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector FVector::operator-(const FVector& InOther) const
{
	return { X - InOther.X, Y - InOther.Y, Z - InOther.Z };
}

/**
 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
 */
FVector FVector::operator*(const float InRatio) const
{
	return { X * InRatio, Y * InRatio, Z * InRatio };
}

/**
 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
 */
FVector& FVector::operator+=(const FVector& InOther)
{
	X += InOther.X;
	Y += InOther.Y;
	Z += InOther.Z;
	return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
}

/**
 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
 */
FVector& FVector::operator-=(const FVector& InOther)
{
	X -= InOther.X;
	Y -= InOther.Y;
	Z -= InOther.Z;
	return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
}

/**
 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
 */

FVector& FVector::operator*=(const float InRatio)
{
	X *= InRatio;
	Y *= InRatio;
	Z *= InRatio;

	return *this;
}

bool FVector::operator==(const FVector& InOther) const
{
	if (X == InOther.X && Y == InOther.Y && Z == InOther.Z)
	{
		return true;
	}
	return false;
}

FArchive& operator<<(FArchive& Ar, FVector& Vector)
{
	Ar << Vector.X;
	Ar << Vector.Y;
	Ar << Vector.Z;
	return Ar;
}

	/**
	 * @brief FVector 기본 생성자
	 */
FVector4::FVector4()
		: X(0), Y(0), Z(0), W(0)
{
}

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const float InX, const float InY, const float InZ, const float InW)
		: X(InX), Y(InY), Z(InZ), W(InW)
{
}


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const FVector4& InOther)
		: X(InOther.X), Y(InOther.Y), Z(InOther.Z), W(InOther.W)
{
}


/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator+(const FVector4& InOtherVector) const
{
	return FVector4(
		X + InOtherVector.X,
		Y + InOtherVector.Y,
		Z + InOtherVector.Z,
		W + InOtherVector.W
	);
}

FVector4 FVector4::operator*(const FMatrix& InMatrix) const
{
	FVector4 Result;
	Result.X = X * InMatrix.Data[0][0] + Y * InMatrix.Data[1][0] + Z * InMatrix.Data[2][0] + W * InMatrix.Data[3][0];
	Result.Y = X * InMatrix.Data[0][1] + Y * InMatrix.Data[1][1] + Z * InMatrix.Data[2][1] + W * InMatrix.Data[3][1];
	Result.Z = X * InMatrix.Data[0][2] + Y * InMatrix.Data[1][2] + Z * InMatrix.Data[2][2] + W * InMatrix.Data[3][2];
	Result.W = X * InMatrix.Data[0][3] + Y * InMatrix.Data[1][3] + Z * InMatrix.Data[2][3] + W * InMatrix.Data[3][3];

	return Result;
}
/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator-(const FVector4& InOtherVector) const
{
	return FVector4(
		X - InOtherVector.X,
		Y - InOtherVector.Y,
		Z - InOtherVector.Z,
		W - InOtherVector.W
	);
}

/**
 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
 */
FVector4 FVector4::operator*(const float InRatio) const
{
	return FVector4(
		X * InRatio,
		Y * InRatio,
		Z * InRatio,
		W * InRatio
	);
}


/**
 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
 */
void FVector4::operator+=(const FVector4& InOtherVector)
{
	X += InOtherVector.X;
	Y += InOtherVector.Y;
	Z += InOtherVector.Z;
	W += InOtherVector.W;
}

/**
 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
 */
void FVector4::operator-=(const FVector4& InOtherVector)
{
	X -= InOtherVector.X;
	Y -= InOtherVector.Y;
	Z -= InOtherVector.Z;
	W -= InOtherVector.W;
}

/**
 * @brief 자신의 벡터에 배율을 곱하는 함수
 */
void FVector4::operator*=(const float Ratio)
{
	X *= Ratio;
	Y *= Ratio;
	Z *= Ratio;
	W *= Ratio;
}

FArchive& operator<<(FArchive& Ar, FVector4& Vector)
{
	Ar << Vector.X;
	Ar << Vector.Y;
	Ar << Vector.Z;
	Ar << Vector.W;
	return Ar;
}

/**
 * @brief FVector2 기본 생성자
 */
FVector2::FVector2()
	: X(0), Y(0)
{
}

/**
 * @brief FVector2의 멤버값을 Param으로 넘기는 생성자
 */
FVector2::FVector2(float InX, float InY)
	: X(InX), Y(InY)
{
}

/**
 * @brief FVector2를 Param으로 넘기는 생성자
 */
FVector2::FVector2(const FVector2& InOther)
	: X(InOther.X), Y(InOther.Y)
{
}

/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector2 FVector2::operator+(const FVector2& InOther) const
{
	return { X + InOther.X, Y + InOther.Y };
}

/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector2 FVector2::operator-(const FVector2& InOther) const
{
	return { X - InOther.X, Y - InOther.Y };
}

/**
 * @brief 자신의 벡터에서 배율을 곱한 백터를 반환하는 함수
 */
FVector2 FVector2::operator*(const float Ratio) const
{
	return { X * Ratio, Y * Ratio };
}

FArchive& operator<<(FArchive& Ar, FVector2& Vector)
{
	Ar << Vector.X;
	Ar << Vector.Y;
	return Ar;
}

