#include "pch.h"
#include "Camera/Public/Camera.h"

void Camera::UpdateMatrix()
{
	/**
	 * @brief View 행렬 연산
	 */
	const float Roll = FVector::GetDegreeToRadian(-Rotation.X);
	const float Pitch = FVector::GetDegreeToRadian(-Rotation.Y);
	const float Yaw = FVector::GetDegreeToRadian(-Rotation.Z);

	FConstants T = FConstants::TranslationMatrix(-Position);
	FConstants R = FConstants::RotationMatrix({ Roll, Pitch, Yaw });
	ViewProjConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FConstants P = FConstants::Identity();
	// | f/aspect   0        0         0 |
	// |    0       f        0         0 |
	// |    0       0   zf/(zf-zn)     1 |
	// |    0       0  -zn*zf/(zf-zn)  0 |
	P.Data[0][0] = F / Aspect;
	P.Data[1][1] = F;
	P.Data[2][2] = FarZ / (FarZ - NearZ);
	P.Data[2][3] = 1.0f;
	P.Data[3][2] = (-NearZ * FarZ) / (FarZ - NearZ);
	P.Data[3][3] = 0.0f;

	ViewProjConstants.Projection = P;
}
