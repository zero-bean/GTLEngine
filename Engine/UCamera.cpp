#include "stdafx.h"
#include "UCamera.h"

// ==== 투영 관련 ====

void UCamera::SetPerspective(float fovY, float aspect, float zn, float zf)
{
	mUseOrtho = false;
	mFovY = fovY; mAspect = aspect; mNearZ = zn; mFarZ = zf;
	UpdateProj();
}

void UCamera::SetPerspectiveDegrees(float fovYDeg, float aspect, float zn, float zf)
{
	mUseOrtho = false;
	mFovY = ToRad(fovYDeg);
	mAspect = (aspect > 0.0f) ? aspect : 1.0f;
	mNearZ = zn; mFarZ = zf;
	UpdateProj();
}

void UCamera::SetOrtho(float w, float h, float zn, float zf)
{
	mUseOrtho = true;
	mOrthoWidth = (w > 0.0f) ? w : 1.0f;
	mOrthoHeight = (h > 0.0f) ? h : 1.0f;
	mNearZ = zn; mFarZ = zf;
	UpdateProj();
}

// ==== 카메라 정보 ====

// 현재 카메라의 로컬 축 3개를 한꺼번에 얻기
void UCamera::GetBasis(FVector& outRight, FVector& outForward, FVector& outUp) const
{
	outRight = GetRight();
	outForward = GetForward();
	outUp = GetUp();
}
FVector UCamera::GetBasis() const
{
	return GetRight(), GetForward(), GetUp();
}



// ==== 카메라 움직임 관련 ====

// target을 바라보도록 (RH, Z-up)
// RH 크로스 사용: Right = forward x Up, Up = right x forward(기존 크로스와 다른 순서)
void UCamera::LookAt(const FVector& eye, const FVector& target, const FVector& up)
{
	mEye = eye;
	FVector F = (target - eye).Normalized();
	mRot = FQuaternion::LookRotation(F, up);
	// pitch 추정
	const float fz = std::clamp(F.Z, -1.0f, 1.0f);
	mPitch = asinf(std::clamp(GetForward().Z, -1.0f, 1.0f));
	// 뷰 갱신
	UpdateView();
}
// 로컬 축 기준 이동 (dx=right, dy=forward, dz=up)
void UCamera::MoveLocal(float dx, float dy, float dz, float deltaTime, bool boost,
	float baseSpeed, float boostMul)
{
	const float speed = baseSpeed * (boost ? boostMul : 1.0f);
	const FVector delta =
		GetRight() * dx +
		GetForward() * dy +
		GetUp() * dz;
	mEye = mEye + delta * (speed * deltaTime);
	UpdateView();
}
// 델타 기반 무제한 마우스룩: 월드 Z로 Yaw, 그 뒤 "Yaw 적용 후 Right"로 Pitch
void UCamera::AddYawPitch(float yawZ, float pitch)
{
	if (yawZ == 0.f && pitch == 0.f) return;

	// 1) 월드 Z로 Yaw
	const FQuaternion qYaw = FQuaternion::FromAxisAngle(FVector(0, 0, 1), yawZ);

	// 2) Pitch 축 = "Yaw 적용 후" Right
	const FQuaternion rotAfterYaw = (qYaw * mRot).Normalized();
	const FVector rightAfterYaw = rotAfterYaw.Rotate(FVector(0, 1, 0)).Normalized();
	const FQuaternion qPitch = FQuaternion::FromAxisAngle(rightAfterYaw, pitch);

	// 합성
	mRot = (qPitch * qYaw * mRot).Normalized();

	UpdateView();
}

void UCamera::GetYawPitch(float& yawZ, float& pitch) const
{
	const FVector f = GetForward(); // Z-up
	pitch = asinf(std::clamp(f.Z, -1.0f, 1.0f));
	yawZ = atan2f(f.Y, f.X);
	if (yawZ > PI) yawZ -= 2.f * PI;
	if (yawZ < -PI) yawZ += 2.f * PI;
}
// TODO : 정확히 잘 작동하는지 모름. 사용할 때 확인 요망
void UCamera::SetYawPitch(float yawZ, float pitch)
{
	// 1) Yaw (월드 Z)
	if (yawZ != 0.0f)
	{
		const FQuaternion qYaw = FQuaternion::FromAxisAngle(FVector(0, 0, 1), yawZ);
		mRot = (qYaw * mRot).Normalized();
	}

	// 2) Pitch (Local Right) — 피치 제한 적용
	if (pitch != 0.0f)
	{
		const float kMax = ToRad(89.0f);
		// 현재 forward로부터 pitch 추정
		const float currPitch = asinf(std::clamp(GetForward().Z, -1.0f, 1.0f));
		float newPitch = currPitch + pitch;
		newPitch = max(-kMax, min(kMax, newPitch));

		const float dp = newPitch - currPitch;
		if (fabsf(dp) > 1e-6f)
		{
			const FVector right = GetRight();
			const FQuaternion qPitch = FQuaternion::FromAxisAngle(right, dp);
			mRot = (qPitch * mRot).Normalized();
		}
	}

	UpdateView();
}

FVector UCamera::GetEulerXYZRad() const
{
	return mRot.GetEulerXYZ();
}


FVector UCamera::GetEulerXYZDeg() const
{
	return mRot.GetEulerXYZDeg();
}

void UCamera::SetEulerXYZRad(float rx, float ry, float rz)
{
	if (bLockRoll) rz = 0.0f;            // 롤 잠그고 싶으면 유지, 아니면 제거/토글
	mRot = FQuaternion::FromEulerXYZ(rx, ry, rz).Normalized();
	// pitch 캐시(편의): forward.z = sin(pitch)
	float fz = std::clamp(mForward.Z, -1.0f, 1.0f);
	mPitch = asinf(fz);
	UpdateView();
}


void UCamera::SetEulerXYZDeg(float rxDeg, float ryDeg, float rzDeg)
{
	SetEulerXYZRad(rxDeg * DegreeToRadian, ryDeg * DegreeToRadian, rzDeg * DegreeToRadian);
}

// ==== 내부 구현(private 함수들) ====

// mRot로부터 mRight, mUp, mForward 갱신

// 투영 행렬 갱신
void UCamera::UpdateProj()
{
	if (!mUseOrtho)
	{
		// RH + row-vector + D3D depth [0,1]
		mProj = FMatrix::PerspectiveFovLHRow(mFovY, mAspect, mNearZ, mFarZ);
	}
	else
	{
		mProj = FMatrix::OrthoLHRow(mOrthoWidth, mOrthoHeight, mNearZ, mFarZ);
	}
}

// 뷰 행렬 갱신
void UCamera::UpdateView()
{
	FVector s = GetRight();    // Right
	FVector u = GetUp();       // Up
	FVector f = GetForward();
	FMatrix V = FMatrix::IdentityMatrix();
	// 회전 성분: 열에 [s u f] 배치 (row-vector 규약)
	V.M[0][0] = s.X; V.M[0][1] = u.X; V.M[0][2] = f.X;
	V.M[1][0] = s.Y; V.M[1][1] = u.Y; V.M[1][2] = f.Y;
	V.M[2][0] = s.Z; V.M[2][1] = u.Z; V.M[2][2] = f.Z;
	// 평행이동(마지막 행)
	V.M[3][0] = -(mEye.Dot(s));
	V.M[3][1] = -(mEye.Dot(u));
	V.M[3][2] = -(mEye.Dot(f));
	V.M[3][3] = 1.0f;
	mView = V;
}