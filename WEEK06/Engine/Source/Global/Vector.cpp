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

FVector FVector::operator/(const float InRatio) const
{
	return { X / InRatio, Y / InRatio, Z / InRatio };
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

FVector& FVector::operator/=(const float InRatio)
{
	X /= InRatio;
	Y /= InRatio;
	Z /= InRatio;

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

FVector4 FVector4::operator*(const FMatrix& m) const
{
	FVector4 out;
	__m128 v = _mm_setr_ps(X, Y, Z, W);

	__m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0)); // X
	__m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1)); // Y
	__m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2)); // Z
	__m128 vw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3)); // W

	__m128 r0 = _mm_mul_ps(vx, _mm_loadu_ps(m.Data[0])); // row0
	__m128 r1 = _mm_mul_ps(vy, _mm_loadu_ps(m.Data[1])); // row1
	__m128 r2 = _mm_mul_ps(vz, _mm_loadu_ps(m.Data[2])); // row2
	__m128 r3 = _mm_mul_ps(vw, _mm_loadu_ps(m.Data[3])); // row3

	__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
	_mm_storeu_ps(&out.X, res);
	return out;
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

