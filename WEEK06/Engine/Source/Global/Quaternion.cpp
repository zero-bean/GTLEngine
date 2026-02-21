#include "pch.h"
#include "Global/Quaternion.h"

FQuaternion FQuaternion::Inverse() const
{
	__m128 q = loadq(*this);
	__m128 conj = _mm_xor_ps(q, _mm_set_ps(-0.0f, -0.0f, -0.0f, 0.0f)); // flip signs of X,Y,Z only
	__m128 sq = _mm_mul_ps(q, q);
	__m128 sum1 = _mm_hadd_ps(sq, sq);
	__m128 sum2 = _mm_hadd_ps(sum1, sum1); // n = X²+Y²+Z²+W²
	float n = _mm_cvtss_f32(sum2);

	FQuaternion out;
	if (n > 1e-8f) {
		__m128 invn = _mm_set1_ps(1.0f / n);
		storeq(out, _mm_mul_ps(conj, invn));
	} else {
		out = FQuaternion{0,0,0,0};
	}
	return out;
}

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(N.X * s, N.Y * s, N.Z * s, cosf(AngleRad * 0.5f));
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float cx = cosf(Radians.X * 0.5f);
	float sx = sinf(Radians.X * 0.5f);
	float cy = cosf(Radians.Y * 0.5f);
	float sy = sinf(Radians.Y * 0.5f);
	float cz = cosf(Radians.Z * 0.5f);
	float sz = sinf(Radians.Z * 0.5f);

	// Yaw-Pitch-Roll (Z, Y, X)
	return FQuaternion(
		sx * cy * cz - cx * sy * sz, // X
		cx * sy * cz + sx * cy * sz, // Y
		cx * cy * sz - sx * sy * cz, // Z
		cx * cy * cz + sx * sy * sz  // W
	);
}

FVector FQuaternion::ToEuler() const
{
	FVector Euler;

	// Roll (X)
	float sinr_cosp = 2.0f * (W * X + Y * Z);
	float cosr_cosp = 1.0f - 2.0f * (X * X + Y * Y);
	Euler.X = atan2f(sinr_cosp, cosr_cosp);

	// Pitch (Y)
	float sinp = 2.0f * (W * Y - Z * X);
	if (fabs(sinp) >= 1)
		Euler.Y = copysignf(PI / 2, sinp); // 90도 고정
	else
		Euler.Y = asinf(sinp);

	// Yaw (Z)
	float siny_cosp = 2.0f * (W * Z + X * Y);
	float cosy_cosp = 1.0f - 2.0f * (Y * Y + Z * Z);
	Euler.Z = atan2f(siny_cosp, cosy_cosp);

	return FVector::GetRadianToDegree(Euler);
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return FQuaternion(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	);
}

void FQuaternion::Normalize()
{
	__m128 q = loadq(*this);
	__m128 sq = _mm_mul_ps(q, q);
	__m128 sum1 = _mm_hadd_ps(sq, sq);
	__m128 sum2 = _mm_hadd_ps(sum1, sum1);
	float mag2 = _mm_cvtss_f32(sum2);
	if (mag2 > 1e-8f) {
		float invMag = 1.0f / sqrtf(mag2);
		__m128 s = _mm_set1_ps(invMag);
		storeq(*this, _mm_mul_ps(q, s));
	}
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FVector FQuaternion::RotateVector(const FVector& v) const
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = (*this) * p * this->Inverse();
	return FVector(r.X, r.Y, r.Z);
}
