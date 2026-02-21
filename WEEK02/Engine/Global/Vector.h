#pragma once

struct FMatrix;

struct FVector
{
	float X;
	float Y;
	float Z;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector();


	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector(float InX, float InY, float InZ);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector(const FVector& InOther);

	void operator=(const FVector4& InOther);
	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector operator+(const FVector& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector operator-(const FVector& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
	 */
	FVector operator*(const float Ratio) const;

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	FVector& operator+=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
	 */
	FVector& operator-=(const FVector& InOther);

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
	 */
	FVector& operator*=(const float Ratio);

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	inline FVector operator-() const { return FVector(-X, -Y, -Z); }


	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	inline float Length() const { return sqrtf(X * X + Y * Y + Z * Z); }

	/**
	 * @brief 자신의 벡터의 각 성분을 제곱하여 더한 값을 반환하는 함수 (루트 사용 X)
	 */
	inline float LengthSquared() const { return (X * X) + (Y * Y) + (Z * Z); }

	/**
	 * @brief 두 벡터를 내적하여 결과의 스칼라 값을 반환하는 함수
	 */
	inline float Dot(const FVector& OtherVector) const { return (X * OtherVector.X) + (Y * OtherVector.Y) + (Z * OtherVector.Z); }

	/**
	 * @brief 두 벡터를 외적한 결과의 벡터 값을 반환하는 함수
	 */
	inline FVector Cross(const FVector& OtherVector) const
	{
		return FVector(
			Z * OtherVector.Y - Y * OtherVector.Z,
			X * OtherVector.Z - Z * OtherVector.X,
			Y * OtherVector.X - X * OtherVector.Y
		);
	}

	/**
	 * @brief 단위 벡터로 변경하는 함수
	 */
	inline void Normalize()
	{
		float Length = sqrt(LengthSquared());
		if (Length > 0.00000001f)
		{
			X /= Length;
			Y /= Length;
			Z /= Length;
		}
	}

	/**
	 * @brief 각도를 라디안으로 변환한 값을 반환하는 함수
	 */
	inline static float GetDegreeToRadian(const float Degree) { return (Degree * Pi) / 180.f; }
	inline static FVector GetDegreeToRadian(const FVector& Rotation)
	{
		return FVector{ (Rotation.X * Pi) / 180.f, (Rotation.Y * Pi) / 180.f, (Rotation.Z * Pi) / 180.f };
	}
	/**
	 * @brief 라디안를 각도로 변환한 값을 반환하는 함수
	 */
	inline static float GetRadianToDegree(const float Radian) { return (Radian * 180.f) / Pi; }
};


struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	/**
	 * @brief FVector 기본 생성자
	 */
	FVector4();

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector4(const float InX, const float InY, const float InZ, const float InW);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector4& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator+(const FVector4& OtherVector) const;

	/**
	 * @brief 벡터와 행렬곱
	 */
	FVector4 operator*(const FMatrix& Matrix) const;
	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator-(const FVector4& OtherVector) const;

	/**
	 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
	 */
	FVector4 operator*(const float Ratio) const;


	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	void operator+=(const FVector4& OtherVector);

	/**
	 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
	 */
	void operator-=(const FVector4& OtherVector);

	/**
	 * @brief 자신의 벡터에 배율을 곱하는 함수
	 */
	void operator*=(const float Ratio);

	inline float Length() const
	{
		return sqrtf(X * X + Y * Y + Z * Z + W * W);
	}

	inline void Normalize()
	{
		float Mag = this->Length();
		X /= Mag;
		Y /= Mag;
		Z /= Mag;
		W /= Mag;
	}


	/**
	 * @brief W성분 무시하고 dot product 진행하는 함수
	 */
	inline float Dot3(const FVector4& OtherVector) const
	{
		return X * OtherVector.X + Y * OtherVector.Y + Z * OtherVector.Z;
	}
	inline float Dot3(const FVector& OtherVector) const
	{
		return X * OtherVector.X + Y * OtherVector.Y + Z * OtherVector.Z;
	}
};
