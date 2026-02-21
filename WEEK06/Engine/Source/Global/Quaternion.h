#pragma once

struct FVector;
struct alignas(16) FQuaternion
{
	float X;
	float Y;
	float Z;
	float W;

	FQuaternion() : X(0), Y(0), Z(0), W(1) {}
	FQuaternion(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}

	static FQuaternion Identity() { return FQuaternion(0, 0, 0, 1); }
	static FQuaternion FromAxisAngle(const FVector& Axis, float AngleRad);

	static FQuaternion FromEuler(const FVector& EulerDeg);
	FVector ToEuler() const;

	FQuaternion operator*(const FQuaternion& Q) const;

	void Normalize();

	FQuaternion Conjugate() const { return FQuaternion(-X, -Y, -Z, W); }
	FQuaternion Inverse() const;
	static FVector RotateVector(const FQuaternion& q, const FVector& v);
	FVector RotateVector(const FVector& v) const;
};

inline __m128 loadq(const FQuaternion& q) {
	return _mm_loadu_ps(&q.X);
}
inline void storeq(FQuaternion& q, __m128 v) {
	_mm_storeu_ps(&q.X, v);
}
