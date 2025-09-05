#pragma once

struct FVector
{
	float X;
	float Y;
	float Z;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector()
		: X(0), Y(0), Z(0)
	{
	}


	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector(float InX, float InY, float InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}

	/**
	 * FVector 간의 내적 함수
	 * @param rhs 연산할 다른 FVector
	 * @return 내적 연산 결과
	 */
	float Dot(const FVector& rhs) const
	{
		return X * rhs.X + Y * rhs.Y + Z * rhs.Z;
	}

	/**
	 * FVector간의 외적 함수
	 * @param rhs 연산할 다른 FVector
	 * @return 외적 연산 결과
	 */
	FVector Cross(const FVector& rhs) const
	{
		return {rhs.Y * Z - rhs.Z * Y, X * rhs.Z - Z * rhs.X, rhs.X * Y - X * rhs.Y};
	}

	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	float Length() const
	{
		return sqrtf(X * X + Y * Y + Z * Z);
	}

	/**
	 * @brief 벡터를 단위벡터로 변환하는 함수
	 */
	void Normalize()
	{
		float VectorLength = Length();
		X /= VectorLength;
		Y /= VectorLength;
		Z /= VectorLength;
	}

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector operator+(const FVector& InOther) const
	{
		return {X + InOther.X, Y + InOther.Y, Z + InOther.Z};
	}

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector operator-(const FVector& InOther) const
	{
		return {X - InOther.X, Y - InOther.Y, Z - InOther.Z};
	}

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	FVector& operator+=(const FVector& InOther)
	{
		X += InOther.X;
		Y += InOther.Y;
		Z += InOther.Z;
		return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
	}

	/**
	 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
	 */
	FVector& operator-=(const FVector& InOther)
	{
		X -= InOther.X;
		Y -= InOther.Y;
		Z -= InOther.Z;
		return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
	}
};

struct FVertexSimple
{
	float x, y, z; // Position
	float r, g, b, a; // Color
};

struct FConstants
{
	FVector Offset;
	float ScaleX;
	float ScaleY;
	float Rotation;
	float Radius; // Triangle일 때 내접원 반지름, 다른 도형은 0
	float Pad[5]; // 16바이트 정렬(총 48바이트)
};
