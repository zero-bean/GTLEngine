#pragma once

struct FVector4;
struct FArchive; // @note: 직렬화 지원용 헤더
struct FMatrix;

struct alignas(16) FVector
{
	float X;
	float Y;
	float Z;

	float& operator[](int Index)
	{
		switch (Index)
		{
		case 0: return X;
		case 1: return Y;
		case 2: return Z;
		}
	}

	const float& operator[](int Index) const
	{
		switch (Index)
		{
		case 0: return X;
		case 1: return Y;
		case 2: return Z;
		}
	}

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
	FVector operator*(float InRatio) const;

	FVector operator/(float InRatio) const;

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
	FVector& operator*=(float InRatio);

	FVector& operator/=(float InRatio);

	bool operator!=(const FVector& Other) const
	{
		return !(*this == Other);
	}

	/**
	 * @brief 자신의 벡터의 각 성분의 부호를 반전한 값을 반환
	 */
	FVector operator-() const { return {-X, -Y, -Z}; }

	bool operator==(const FVector& InOther) const;

	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	float LengthSquared() const
	{
		__m128 v = _mm_set_ps(0, Z, Y, X);
		__m128 mul = _mm_mul_ps(v, v);
		__m128 sum1 = _mm_hadd_ps(mul, mul);
		__m128 sum2 = _mm_hadd_ps(sum1, sum1);
		return _mm_cvtss_f32(sum2);
	}

	float Length() const
	{
		return sqrtf(LengthSquared());
	}

	/**
	 * @brief 두 벡터를 내적하여 결과의 스칼라 값을 반환하는 함수
	 */
	float Dot(const FVector& b) const
	{
		__m128 v1 = _mm_set_ps(0, Z, Y, X);
		__m128 v2 = _mm_set_ps(0, b.Z, b.Y, b.X);
		__m128 dp = _mm_dp_ps(v1, v2, 0x71);
		return _mm_cvtss_f32(dp);
	}

	/**
	 * @brief 두 벡터를 외적한 결과의 벡터 값을 반환하는 함수
	 */
	FVector Cross(const FVector& b) const
	{
		__m128 a = _mm_set_ps(0.0f, Z, Y, X);
		__m128 bvec = _mm_set_ps(0.0f, b.Z, b.Y, b.X);

		// Shuffle to get (Y,Z,X) pattern
		__m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,0,2,1));
		__m128 b_yzx = _mm_shuffle_ps(bvec, bvec, _MM_SHUFFLE(3,0,2,1));

		// Compute right-handed cross
		__m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, bvec));
		__m128 rh = _mm_shuffle_ps(c, c, _MM_SHUFFLE(3,0,2,1));

		// Negate for left-handed system
		__m128 lh = _mm_xor_ps(rh, _mm_set1_ps(-0.0f)); // flip sign bit

		FVector out;
		_mm_storeu_ps(&out.X, lh);
		return out;
	}

	/**
	 * @brief 단위 벡터로 변경하는 함수
	 */
	static inline void store3(__m128 v, float& x, float& y, float& z) {
		alignas(16) float tmp[4];
		_mm_store_ps(tmp, v); // requires 16B alignment of tmp (stack is fine)
		x = tmp[0]; y = tmp[1]; z = tmp[2];
	}
	void Normalize()
	{
		// Load as [X, Y, Z, 0]
		__m128 v = _mm_setr_ps(X, Y, Z, 0.0f);

		// Length^2 (XYZ only). If you lack SSE4.1, replace with mul+hadd fallback.
		__m128 dp = _mm_dp_ps(v, v, 0x71);
		float lenSq = _mm_cvtss_f32(dp);

		if (lenSq > 1e-16f) {
			// Compute scalar 1/sqrt(lenSq), then broadcast to all lanes
			float invLenScalar = 1.0f / std::sqrt(lenSq);
			__m128 invLen = _mm_set1_ps(invLenScalar);

			__m128 n = _mm_mul_ps(v, invLen);
			store3(n, X, Y, Z);
		}
	}

	/**
	 * @brief 각도를 라디안으로 변환한 값을 반환하는 함수
	 */
	static float GetDegreeToRadian(const float InDegree) { return (InDegree * PI) / 180.f; }
	static FVector GetDegreeToRadian(const FVector& deg)
	{
		__m128 v = _mm_set_ps(0.0f, deg.Z, deg.Y, deg.X);
		__m128 scale = _mm_set1_ps(PI / 180.0f);
		__m128 rad = _mm_mul_ps(v, scale);

		FVector out;
		_mm_storeu_ps(&out.X, rad);
		return out;
	}

	/**
	 * @brief 라디안를 각도로 변환한 값을 반환하는 함수
	 */
	static float GetRadianToDegree(const float Radian) { return (Radian * 180.f) / PI; }
	static FVector GetRadianToDegree(const FVector& rad)
	{
		__m128 v = _mm_set_ps(0.0f, rad.Z, rad.Y, rad.X);
		__m128 scale = _mm_set1_ps(180.0f / PI);
		__m128 deg = _mm_mul_ps(v, scale);

		FVector out;
		_mm_storeu_ps(&out.X, deg);
		return out;
	}

	// Constant Vector (definition from UE5)
	static FVector ZeroVector() { return {0.0f, 0.0f, 0.0f}; }
	static FVector OneVector() { return {1.0f, 1.0f, 1.0f}; }
	static FVector ForwardVector() { return {1.0f, 0.0f, 0.0f}; }
	static FVector BackwardVector() { return {-1.0f, 0.0f, 0.0f}; }
	static FVector UpVector() { return {0.0f, 0.0f, 1.0f}; }
	static FVector DownVector() { return {0.0f, 0.0f, -1.0f}; }
	static FVector RightVector() { return {0.0f, 1.0f, 0.0f}; }
	static FVector LeftVector() { return {0.0f, -1.0f, 0.0f}; }
	static FVector XAxisVector() { return {1.0f, 0.0f, 0.0f}; }
	static FVector YAxisVector() { return {0.0f, 1.0f, 0.0f}; }
	static FVector ZAxisVector() { return {0.0f, 0.0f, 1.0f}; }

	[[nodiscard]] static FVector Zero() { return ZeroVector(); }
	[[nodiscard]] static FVector One() { return OneVector(); }
	[[nodiscard]] static FVector UnitX() { return XAxisVector(); }
	[[nodiscard]] static FVector UnitY() { return YAxisVector(); }
	[[nodiscard]] static FVector UnitZ() { return ZAxisVector(); }
};

FArchive& operator<<(FArchive& Ar, FVector& Vector);

struct FVector2
{
	float X;
	float Y;

	/**
	 * @brief FVector2 기본 생성자
	 */
	FVector2();

	/**
	 * @brief FVector2의 멤버값을 Param으로 넘기는 생성자
	 */
	FVector2(float InX, float InY);

	/**
	 * @brief FVector2를 Param으로 넘기는 생성자
	 */
	FVector2(const FVector2& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator+(const FVector2& InOther) const;

	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector2 operator-(const FVector2& InOther) const;

	/**
	 * @brief 자신의 벡터에서 배율을 곱한 백터를 반환하는 함수
	 */
	FVector2 operator*(const float Ratio) const;

	/**
	 * @brief 벡터의 길이 연산 함수
	 * @return 벡터의 길이
	 */
	inline float Length() const { return sqrtf(X * X + Y * Y); }

	/**
	 * @brief 자신의 벡터의 각 성분을 제곱하여 더한 값을 반환하는 함수 (루트 사용 X)
	 */
	inline float LengthSquared() const { return (X * X) + (Y * Y); }
};

FArchive& operator<<(FArchive& Ar, FVector2& Vector);

struct alignas(16) FVector4
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
	FVector4(float InX, float InY, float InZ, float InW);


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
	FVector4(const FVector4& InOther);

	/**
	 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator+(const FVector4& InOtherVector) const;

	/**
	 * @brief 벡터와 행렬곱
	 */
	FVector4 operator*(const FMatrix& InMatrix) const;
	/**
	 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
	 */
	FVector4 operator-(const FVector4& InOtherVector) const;

	/**
	 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
	 */
	FVector4 operator*(float InRatio) const;


	/**
	 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
	 */
	void operator+=(const FVector4& InOtherVector);

	/**
	 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
	 */
	void operator-=(const FVector4& InOtherVector);

	/**
	 * @brief 자신의 벡터에 배율을 곱하는 함수
	 */
	void operator*=(float Ratio);

	float Length() const
	{
		return sqrtf(X * X + Y * Y + Z * Z + W * W);
	}

	float LengthSquared() const
	{
		return (X * X) + (Y * Y) + (Z * Z) + (W * W);
	}

	void Normalize()
	{
		float Magnitude = this->Length();
		X /= Magnitude;
		Y /= Magnitude;
		Z /= Magnitude;
		W /= Magnitude;
	}

	float Dot(const FVector4& Other) const
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z + W * Other.W;
	}


	/**
	 * @brief W 성분 무시하고 dot product 진행하는 함수
	 */
	float Dot3(const FVector4& InOtherVector) const
	{
		return X * InOtherVector.X + Y * InOtherVector.Y + Z * InOtherVector.Z;
	}

	float Dot3(const FVector& InOtherVector) const
	{
		return X * InOtherVector.X + Y * InOtherVector.Y + Z * InOtherVector.Z;
	}




	float& operator[](int Index)
	{
		switch (Index)
		{
		case 0: return X;
		case 1: return Y;
		case 2: return Z;
		case 3: return W;
		}
	}

	const float& operator[](int Index) const
	{
		switch (Index)
		{
		case 0: return X;
		case 1: return Y;
		case 2: return Z;
		case 3: return W;
		}
	}


	// Constant Vector (definition from UE5)
	static FVector4 ZeroVector() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
	static FVector4 OneVector() { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
	static FVector4 ForwardVector() { return { 1.0f, 0.0f, 0.0f, 1.0f }; }
	static FVector4 BackwardVector() { return { -1.0f, 0.0f, 0.0f, 1.0f }; }
	static FVector4 UpVector() { return { 0.0f, 0.0f, 1.0f, 1.0f }; }
	static FVector4 DownVector() { return { 0.0f, 0.0f, -1.0f, 1.0f }; }
	static FVector4 RightVector() { return { 0.0f, 1.0f, 0.0f, 1.0f }; }
	static FVector4 LeftVector() { return { 0.0f, -1.0f, 0.0f, 1.0f }; }
	static FVector4 XAxisVector() { return { 1.0f, 0.0f, 0.0f, 1.0f }; }
	static FVector4 YAxisVector() { return { 0.0f, 1.0f, 0.0f, 1.0f }; }
	static FVector4 ZAxisVector() { return { 0.0f, 0.0f, 1.0f, 1.0f }; }

	[[nodiscard]] static FVector4 Zero() { return ZeroVector(); }
	[[nodiscard]] static FVector4 One() { return OneVector(); }
	[[nodiscard]] static FVector4 UnitX() { return XAxisVector(); }
	[[nodiscard]] static FVector4 UnitY() { return YAxisVector(); }
	[[nodiscard]] static FVector4 UnitZ() { return ZAxisVector(); }
};

FArchive& operator<<(FArchive& Ar, FVector4& Vector);
