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
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector(const FVector& InOther)
		: X(InOther.X), Y(InOther.Y), Z(InOther.Z)
	{
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
	 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
     */
	FVector operator*(const float Ratio) const
	{
		return { X * Ratio, Y * Ratio, Z * Ratio };
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

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
	 */
	FVector& operator-=(const float Ratio)
	{
		X *= Ratio;
		Y *= Ratio;
		Z *= Ratio;
	}

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	FVector operator-() const { return FVector(-X, -Y, -Z); }


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
			Y * OtherVector.Z - Z * OtherVector.Y,
			Z * OtherVector.X - X * OtherVector.Z,
			X * OtherVector.Y - Y * OtherVector.X
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
	FVector4()
		: X(0), Y(0), Z(0), W(0)
	{
	}

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector4(const float InX, const float InY, const float InZ, const float InW)
		: X(InX), Y(InY), Z(InZ), W(InW)
	{
	}


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector4& InOther)
		: X(InOther.X), Y(InOther.Y), Z(InOther.Z), W(InOther.W)
	{
	}


	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator+(const FVector4& OtherVector) const
	{
		return FVector4(
			X + OtherVector.X,
			Y + OtherVector.Y,
			Z + OtherVector.Z,
			W + OtherVector.W
		);
	}

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator-(const FVector4& OtherVector) const
	{
		return FVector4(
			X - OtherVector.X,
			Y - OtherVector.Y,
			Z - OtherVector.Z,
			W - OtherVector.W
		);
	}

	/**
	 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
	 */
	FVector4 operator*(const float Ratio) const
	{
		return FVector4(
			X * Ratio,
			Y * Ratio,
			Z * Ratio,
			W * Ratio
		);
	}

	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	void operator+=(const FVector4& OtherVector)
	{
		X += OtherVector.X;
		Y += OtherVector.Y;
		Z += OtherVector.Z;
		W += OtherVector.W;
	}

	/**
	 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
	 */
	void operator-=(const FVector4& OtherVector)
	{
		X -= OtherVector.X;
		Y -= OtherVector.Y;
		Z -= OtherVector.Z;
		W -= OtherVector.W;
	}

	/**
	 * @brief 자신의 벡터에 배율을 곱하는 함수
	 */
	void operator*=(const float Ratio)
	{
		X *= Ratio;
		Y *= Ratio;
		Z *= Ratio;
		W *= Ratio;
	}
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FConstants
{
	/**
	* @brief 4x4 float 타입의 행렬
	*/
	float Data[4][4];


	/**
	* @brief float 타입의 배열을 사용한 FConstants의 기본 생성자
	*/
	FConstants()
		: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
	{
	}


	/**
	* @brief float 타입의 param을 사용한 FConstants의 기본 생성자
	*/
	FConstants(
		float M00, float M01, float M02, float M03,
		float M10, float M11, float M12, float M13,
		float M20, float M21, float M22, float M23,
		float M30, float M31, float M32, float M33)
		: Data{ {M00,M01,M02,M03},
				{M10,M11,M12,M13},
				{M20,M21,M22,M23},
				{M30,M31,M32,M33} }
	{
	}

	/**
	* @brief 항등행렬
	*/
	static FConstants Identity()
	{
		return FConstants(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}


	/**
	* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
	*/
	FConstants operator*(const FConstants& InOtherMatrix)
	{
		FConstants Result;

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				for (int k = 0; k < 4; ++k)
				{
					Result.Data[i][j] += Data[i][k] * InOtherMatrix.Data[k][j];
				}
			}
		}

		return Result;
	}

	/**
	* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FConstants TranslationMatrix(const FVector& InOtherVector)
	{
		FConstants Result = FConstants::Identity();
		Result.Data[3][0] = InOtherVector.X;
		Result.Data[3][1] = InOtherVector.Y;
		Result.Data[3][2] = InOtherVector.Z;
		Result.Data[3][3] = 1;

		return Result;
	}

	/**
	* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FConstants ScaleMatrix(const FVector& InOtherVector)
	{
		FConstants Result = FConstants::Identity();
		Result.Data[0][0] = InOtherVector.X;
		Result.Data[1][1] = InOtherVector.Y;
		Result.Data[2][2] = InOtherVector.Z;
		Result.Data[3][3] = 1;

		return Result;
	}

	/**
	* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FConstants RotationMatrix(const FVector& InOtherVector)
	{
		return RotationX(InOtherVector.X) * RotationY(InOtherVector.Y) * RotationZ(InOtherVector.Z);
	}

	/**
	* @brief X의 회전 정보를 행렬로 변환
	*/
	static FConstants RotationX(float Radian)
	{
		FConstants Result = FConstants::Identity();
		const float C = std::cosf(Radian);
		const float S = std::sinf(Radian);

		Result.Data[1][1] = C;
		Result.Data[1][2] = S;
		Result.Data[2][1] = -S;
		Result.Data[2][2] = C;

		return Result;
	}

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FConstants RotationY(float Radian)
	{
		FConstants Result = FConstants::Identity();
		const float C = std::cosf(Radian);
		const float S = std::sinf(Radian);

		Result.Data[0][0] = C;
		Result.Data[0][2] = -S;
		Result.Data[2][0] = S;
		Result.Data[2][2] = C;

		return Result;
	}

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FConstants RotationZ(float Radian)
	{
		FConstants Result = FConstants::Identity();
		const float C = std::cosf(Radian);
		const float S = std::sinf(Radian);

		Result.Data[0][0] = C;
		Result.Data[0][1] = S;
		Result.Data[1][0] = -S;
		Result.Data[1][1] = C;

		return Result;
	}
};

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FConstants::Identity();
		Projection = FConstants::Identity();
	}

	FConstants View;
	FConstants Projection;
};
