#include "pch.h"


FMatrix FMatrix::UEToDx = FMatrix(
	{
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

FMatrix FMatrix::DxToUE = FMatrix(
	{
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
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

FMatrix::FMatrix(const FVector& x, const FVector& y, const FVector& z)
	:Data{ {x.X, x.Y, x.Z, 0.0f},
			{y.X, y.Y, y.Z, 0.0f},
			{z.X, z.Y, z.Z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

FMatrix::FMatrix(const FVector4& x, const FVector4& y, const FVector4& z)
	:Data{ {x.X,x.Y,x.Z, x.W},
			{y.X,y.Y,y.Z,y.W},
			{z.X,z.Y,z.Z,z.W},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

/**
* @brief 항등행렬
*/
FMatrix FMatrix::Identity()
{
	return FMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix)
{
	FMatrix Result;

	for (int i = 0; i < 4; ++i)
	{
		// Load the row from the left matrix (this)
		__m128 a = _mm_loadu_ps(Data[i]); // row i

		// Broadcast each element of row 'i'
		__m128 a0 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0,0,0,0));
		__m128 a1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1,1,1,1));
		__m128 a2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2,2,2,2));
		__m128 a3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,3,3,3));

		// Multiply each broadcast with the corresponding row of the right matrix
		__m128 r0 = _mm_mul_ps(a0, _mm_loadu_ps(InOtherMatrix.Data[0]));
		__m128 r1 = _mm_mul_ps(a1, _mm_loadu_ps(InOtherMatrix.Data[1]));
		__m128 r2 = _mm_mul_ps(a2, _mm_loadu_ps(InOtherMatrix.Data[2]));
		__m128 r3 = _mm_mul_ps(a3, _mm_loadu_ps(InOtherMatrix.Data[3]));

		// Sum them together
		__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));

		// Store into Result row
		_mm_storeu_ps(Result.Data[i], res);
	}
	return Result;
}

FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix) const
{
	FMatrix Result;

	for (int i = 0; i < 4; ++i)
	{
		// Load the row from the left matrix (this)
		__m128 a = _mm_loadu_ps(Data[i]); // row i

		// Broadcast each element of row 'i'
		__m128 a0 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0,0,0,0));
		__m128 a1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1,1,1,1));
		__m128 a2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2,2,2,2));
		__m128 a3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,3,3,3));

		// Multiply each broadcast with the corresponding row of the right matrix
		__m128 r0 = _mm_mul_ps(a0, _mm_loadu_ps(InOtherMatrix.Data[0]));
		__m128 r1 = _mm_mul_ps(a1, _mm_loadu_ps(InOtherMatrix.Data[1]));
		__m128 r2 = _mm_mul_ps(a2, _mm_loadu_ps(InOtherMatrix.Data[2]));
		__m128 r3 = _mm_mul_ps(a3, _mm_loadu_ps(InOtherMatrix.Data[3]));

		// Sum them together
		__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));

		// Store into Result row
		_mm_storeu_ps(Result.Data[i], res);
	}
	return Result;
}

void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this = (*this) * InOtherMatrix;
}

FMatrix FMatrix::operator+(const FMatrix& Other) const
{
	FMatrix Result;
	for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) Result.Data[i][j] = Data[i][j] + Other.Data[i][j];
	return Result;
}

FMatrix& FMatrix::operator+=(const FMatrix& Other)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			Data[i][j] += Other.Data[i][j];
		}
	}
	return *this;
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = InOtherVector.X;
	Result.Data[3][1] = InOtherVector.Y;
	Result.Data[3][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = -InOtherVector.X;
	Result.Data[3][1] = -InOtherVector.Y;
	Result.Data[3][2] = -InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = InOtherVector.X;
	Result.Data[1][1] = InOtherVector.Y;
	Result.Data[2][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}
FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = 1 / InOtherVector.X;
	Result.Data[1][1] = 1 / InOtherVector.Y;
	Result.Data[2][2] = 1 / InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// 1. 라디안 회전값으로부터 sinf, cosf 값 계산
	const float sp = sinf(InOtherVector.X); // Pitch
	const float cp = cosf(InOtherVector.X);
	const float sy = sinf(InOtherVector.Y); // Yaw
	const float cy = cosf(InOtherVector.Y);
	const float sr = sinf(InOtherVector.Z); // Roll
	const float cr = cosf(InOtherVector.Z);

	// 2. 단위 행렬로 초기화
	FMatrix Result = FMatrix::Identity();

	// 2. 회전 행렬 계산 (Rx * Ry * Rz, 오른손 좌표계 기준)
	// 첫 번째 행
	Result.Data[0][0] = cy * cr;
	Result.Data[0][1] = cy * sr;
	Result.Data[0][2] = -sy;

	// 두 번째 행
	Result.Data[1][0] = sp * sy * cr - cp * sr;
	Result.Data[1][1] = sp * sy * sr + cp * cr;
	Result.Data[1][2] = sp * cy;

	// 세 번째 행
	Result.Data[2][0] = cp * sy * cr + sp * sr;
	Result.Data[2][1] = cp * sy * sr - sp * cr;
	Result.Data[2][2] = cp * cy;

	return Result;
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	//return RotationZ(yaw) * RotationY(pitch)* RotationX(roll);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	return RotationZ(-roll) * RotationY(-yaw) * RotationX(-pitch);
}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
	FMatrix Result = FMatrix::Identity();
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
FMatrix FMatrix::RotationY(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][2] = -S;
	Result.Data[2][0] = S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Z의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][1] = S;
	Result.Data[1][0] = -S;
	Result.Data[1][1] = C;

	return Result;
}

//
FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	// FMatrix T = TranslationMatrix(Location);
	// FMatrix R = RotationMatrix(Rotation);
	// FMatrix S = ScaleMatrix(Scale);
	// FMatrix modelMatrix = S * R * T;
	//
	// // Dx11 y-up 왼손좌표계에서 정의된 물체의 정점을 UE z-up 왼손좌표계로 변환
	// return  FMatrix::UEToDx * modelMatrix;

	// 1. 회전값(Rotation)으로부터 sin, cos 값 미리 계산 (회전 순서: X -> Y -> Z)
	const float pitch = Rotation.X;
	const float yaw   = Rotation.Y;
	const float roll  = Rotation.Z;

	const float sp = sinf(pitch);
	const float cp = cosf(pitch);
	const float sy = sinf(yaw);
	const float cy = cosf(yaw);
	const float sr = sinf(roll);
	const float cr = cosf(roll);

	FMatrix Result;

	// UEToDx * Result 해줘야 하기 때문에 순서만 바꿈
	// FinalResult의 첫 번째 행 (원래 S*R*T의 두 번째 행)
	Result.Data[0][0] = Scale.Y * (sp * sy * cr - cp * sr);
	Result.Data[0][1] = Scale.Y * (sp * sy * sr + cp * cr);
	Result.Data[0][2] = Scale.Y * (sp * cy);
	Result.Data[0][3] = 0.0f;

	// FinalResult의 두 번째 행 (원래 S*R*T의 세 번째 행)
	Result.Data[1][0] = Scale.Z * (cp * sy * cr + sp * sr);
	Result.Data[1][1] = Scale.Z * (cp * sy * sr - sp * cr);
	Result.Data[1][2] = Scale.Z * (cp * cy);
	Result.Data[1][3] = 0.0f;

	// FinalResult의 세 번째 행 (원래 S*R*T의 첫 번째 행)
	Result.Data[2][0] = Scale.X * (cy * cr);
	Result.Data[2][1] = Scale.X * (cy * sr);
	Result.Data[2][2] = Scale.X * -sy;
	Result.Data[2][3] = 0.0f;

	// FinalResult의 네 번째 행 (원래 S*R*T의 네 번째 행)
	Result.Data[3][0] = Location.X;
	Result.Data[3][1] = Location.Y;
	Result.Data[3][2] = Location.Z;
	Result.Data[3][3] = 1.0f;

	return Result;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	// UE 좌표계로 변환된 물체의 정점을 원래의 Dx 11 왼손좌표계 정점으로 변환
	return modelMatrixInverse * FMatrix::DxToUE;
}

FVector4 FMatrix::VectorMultiply(const FVector4& v, const FMatrix& m)
{
	FVector4 out;

	// Load the vector [X,Y,Z,W]
	__m128 vec = _mm_loadu_ps(&v.X);

	// Broadcast each component
	__m128 vx = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)); // X
	__m128 vy = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)); // Y
	__m128 vz = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)); // Z
	__m128 vw = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3,3,3,3)); // W

	// Multiply with each row of the matrix
	__m128 r0 = _mm_mul_ps(vx, _mm_loadu_ps(m.Data[0])); // X * row0
	__m128 r1 = _mm_mul_ps(vy, _mm_loadu_ps(m.Data[1])); // Y * row1
	__m128 r2 = _mm_mul_ps(vz, _mm_loadu_ps(m.Data[2])); // Z * row2
	__m128 r3 = _mm_mul_ps(vw, _mm_loadu_ps(m.Data[3])); // W * row3

	// Sum them together
	__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));

	// Store result
	_mm_storeu_ps(&out.X, res);

	return out;
}

FVector FMatrix::VectorMultiply(const FVector& v, const FMatrix& m)
{
	FVector out;

	// Load vector [X, Y, Z, 0]
	__m128 vec = _mm_set_ps(0.0f, v.Z, v.Y, v.X);

	// Broadcast each component
	__m128 vx = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)); // X
	__m128 vy = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)); // Y
	__m128 vz = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)); // Z

	// Multiply with the first 3 rows of the matrix
	__m128 r0 = _mm_mul_ps(vx, _mm_loadu_ps(m.Data[0])); // X * row0
	__m128 r1 = _mm_mul_ps(vy, _mm_loadu_ps(m.Data[1])); // Y * row1
	__m128 r2 = _mm_mul_ps(vz, _mm_loadu_ps(m.Data[2])); // Z * row2

	// Sum them
	__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), r2);

	// Store result (only XYZ matter)
	_mm_storeu_ps(&out.X, res);

	return out;
}

FMatrix FMatrix::Transpose() const
{
	FMatrix out;

	__m128 row0 = _mm_loadu_ps(Data[0]);
	__m128 row1 = _mm_loadu_ps(Data[1]);
	__m128 row2 = _mm_loadu_ps(Data[2]);
	__m128 row3 = _mm_loadu_ps(Data[3]);

	_MM_TRANSPOSE4_PS(row0, row1, row2, row3);

	_mm_storeu_ps(out.Data[0], row0);
	_mm_storeu_ps(out.Data[1], row1);
	_mm_storeu_ps(out.Data[2], row2);
	_mm_storeu_ps(out.Data[3], row3);

	return out;
}

float FMatrix::Determinant() const
{
	return Data[0][0] * (Data[1][1] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[1][2] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) + Data[1][3] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1]))
		- Data[0][1] * (Data[1][0] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[1][2] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[1][3] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0]))
		+ Data[0][2] * (Data[1][0] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) - Data[1][1] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[1][3] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0]))
		- Data[0][3] * (Data[1][0] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1]) - Data[1][1] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0]) + Data[1][2] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0]));
}

FMatrix FMatrix::Inverse() const
{
	FMatrix Inv = {};
	float Det = Determinant();
	if (fabs(Det) < std::numeric_limits<float>::epsilon()) { return FMatrix::Identity(); }
	float InvDet = 1.0f / Det;
	Inv.Data[0][0] = (Data[1][1] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[1][2] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) + Data[1][3] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1])) * InvDet;
	Inv.Data[0][1] = -(Data[0][1] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[0][2] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) + Data[0][3] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1])) * InvDet;
	Inv.Data[0][2] = (Data[0][1] * (Data[1][2] * Data[3][3] - Data[1][3] * Data[3][2]) - Data[0][2] * (Data[1][1] * Data[3][3] - Data[1][3] * Data[3][1]) + Data[0][3] * (Data[1][1] * Data[3][2] - Data[1][2] * Data[3][1])) * InvDet;
	Inv.Data[0][3] = -(Data[0][1] * (Data[1][2] * Data[2][3] - Data[1][3] * Data[2][2]) - Data[0][2] * (Data[1][1] * Data[2][3] - Data[1][3] * Data[2][1]) + Data[0][3] * (Data[1][1] * Data[2][2] - Data[1][2] * Data[2][1])) * InvDet;
	Inv.Data[1][0] = -(Data[1][0] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[1][2] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[1][3] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0])) * InvDet;
	Inv.Data[1][1] = (Data[0][0] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) - Data[0][2] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[0][3] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0])) * InvDet;
	Inv.Data[1][2] = -(Data[0][0] * (Data[1][2] * Data[3][3] - Data[1][3] * Data[3][2]) - Data[0][2] * (Data[1][0] * Data[3][3] - Data[1][3] * Data[3][0]) + Data[0][3] * (Data[1][0] * Data[3][2] - Data[1][2] * Data[3][0])) * InvDet;
	Inv.Data[1][3] = (Data[0][0] * (Data[1][2] * Data[2][3] - Data[1][3] * Data[2][2]) - Data[0][2] * (Data[1][0] * Data[2][3] - Data[1][3] * Data[2][0]) + Data[0][3] * (Data[1][0] * Data[2][2] - Data[1][2] * Data[2][0])) * InvDet;
	Inv.Data[2][0] = (Data[1][0] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) - Data[1][1] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[1][3] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0])) * InvDet;
	Inv.Data[2][1] = -(Data[0][0] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) - Data[0][1] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) + Data[0][3] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0])) * InvDet;
	Inv.Data[2][2] = (Data[0][0] * (Data[1][1] * Data[3][3] - Data[1][3] * Data[3][1]) - Data[0][1] * (Data[1][0] * Data[3][3] - Data[1][3] * Data[3][0]) + Data[0][3] * (Data[1][0] * Data[3][1] - Data[1][1] * Data[3][0])) * InvDet;
	Inv.Data[2][3] = -(Data[0][0] * (Data[1][1] * Data[2][3] - Data[1][3] * Data[2][1]) - Data[0][1] * (Data[1][0] * Data[2][3] - Data[1][3] * Data[2][0]) + Data[0][3] * (Data[1][0] * Data[2][1] - Data[1][1] * Data[2][0])) * InvDet;
	Inv.Data[3][0] = -(Data[1][0] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1]) - Data[1][1] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0]) + Data[1][2] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0])) * InvDet;
	Inv.Data[3][1] = (Data[0][0] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1]) - Data[0][1] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0]) + Data[0][2] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0])) * InvDet;
	Inv.Data[3][2] = -(Data[0][0] * (Data[1][1] * Data[3][2] - Data[1][2] * Data[3][1]) - Data[0][1] * (Data[1][0] * Data[3][2] - Data[1][2] * Data[3][0]) + Data[0][2] * (Data[1][0] * Data[3][1] - Data[1][1] * Data[3][0])) * InvDet;
	Inv.Data[3][3] = (Data[0][0] * (Data[1][1] * Data[2][2] - Data[1][2] * Data[2][1]) - Data[0][1] * (Data[1][0] * Data[2][2] - Data[1][2] * Data[2][0]) + Data[0][2] * (Data[1][0] * Data[2][1] - Data[1][1] * Data[2][0])) * InvDet;
	return Inv;
}

