#include "pch.h"
#include "Camera/Public/Camera.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"  

void Camera::Update()
{
	const float DeltaTime = UTimeManager::GetInstance().GetDeltaTime();
	const UInputManager& Input = UInputManager::GetInstance();


	const float Roll = FVector::GetDegreeToRadian(Rotation.X);
	const float Yaw = FVector::GetDegreeToRadian(Rotation.Y);
	const float Pitch = FVector::GetDegreeToRadian(Rotation.Z);

	Forward = {
		std::cos(Roll) * std::sin(Yaw), // X
		std::sin(Roll),                 // Y
		std::cos(Roll) * std::cos(Yaw)  // Z
	};
	Forward.Normalize();

	/**
	 * @brief 마우스 우클릭을 하고 있는 동안 카메라 제어가 가능합니다.
	 */
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		/**
		 * @brief W, A, S, D 는 각각 카메라의 상, 하, 좌, 우 이동을 담당합니다.
		 */

		FVector Move = { 0,0,0 };

		
		FVector Up = { 0,1,0 };
		FVector Right = Forward.Cross(Up);

		if (Input.IsKeyDown(EKeyInput::A)) { Move = -Right; }
		else if (Input.IsKeyDown(EKeyInput::D)) { Move = Right; }
		else if (Input.IsKeyDown(EKeyInput::W)) { Move = Forward; }
		else if (Input.IsKeyDown(EKeyInput::S)) { Move = -Forward; }
		else if (Input.IsKeyDown(EKeyInput::Q)) { Move = -Up; }
		else if (Input.IsKeyDown(EKeyInput::E)) { Move = Up; }
		Move.Normalize();
		Position += Move * CameraSpeed * DeltaTime;

		/**
		* @brief 마우스 위치 변화량을 감지하여 카메라의 회전을 담당합니다.
		*/
		const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
		Rotation.X += MouseDelta.Y * KeySensitivityDegPerPixel;
		Rotation.Y += MouseDelta.X * KeySensitivityDegPerPixel;

		// Pitch 클램프(짐벌 플립 방지)
		if (Rotation.X > 89.0f) Rotation.X = 89.0f;
		if (Rotation.X < -89.0f) Rotation.X = -89.0f;

		// Yaw 래핑(값이 무한히 커지지 않도록)
		if (Rotation.Y > 180.0f)  Rotation.Y -= 360.0f;
		if (Rotation.Y < -180.0f) Rotation.Y += 360.0f;
	}
}

void Camera::UpdateMatrix()
{
	/**
	 * @brief View 행렬 연산
	 */

	FMatrix T = FMatrix::TranslationMatrixInverse(Position);
	FMatrix R = FMatrix::RotationMatrixInverse(FVector::GetDegreeToRadian(Rotation));
	ViewProjConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FMatrix P = FMatrix::Identity(); 
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

const FViewProjConstants Camera::GetFViewProjConstantsInverse() const
{
	FViewProjConstants ViewProjConstantsInverse;
	FMatrix R = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(Rotation));
	FMatrix T = FMatrix::TranslationMatrix(Position);
	ViewProjConstantsInverse.View = R * T;

	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);  

	FMatrix P = FMatrix::Identity();
	// | aspect/f   0        0         0 |
	// |    0       1/f        0         0 |
	// |    0       0         0     -(zf - zn) / (zn * zf) |
	// |    0       0        1         zf / (zn*zf) |
	P.Data[0][0] = Aspect / F;
	P.Data[1][1] = 1/F;
	P.Data[2][2] = 0;
	P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
	P.Data[3][2] = 1;
	P.Data[3][3] = FarZ/(NearZ*FarZ);

	ViewProjConstantsInverse.Projection = P;
	return ViewProjConstantsInverse;
}
