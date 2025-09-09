#include "pch.h"
#include "Camera/Public/Camera.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Render/Renderer/Public/Renderer.h"

void Camera::Update()
{
	const UInputManager& Input = UInputManager::GetInstance();

	FVector4 Forward4 = FVector4(0, 0, 1, 1) * FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
	Forward = FVector(Forward4.X, Forward4.Y, Forward4.Z);
	Up = FVector(0, 1, 0);
	Right = Forward.Cross(Up);

	/**
	 * @brief 마우스 우클릭을 하고 있는 동안 카메라 제어가 가능합니다.
	 */
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		/**
		 * @brief W, A, S, D 는 각각 카메라의 상, 하, 좌, 우 이동을 담당합니다.
		 */
		FVector Direction = { 0,0,0 };

		if (Input.IsKeyDown(EKeyInput::A)) { Direction += -Right; }
		if (Input.IsKeyDown(EKeyInput::D)) { Direction += Right; }
		if (Input.IsKeyDown(EKeyInput::W)) { Direction += Forward; }
		if (Input.IsKeyDown(EKeyInput::S)) { Direction += -Forward; }
		if (Input.IsKeyDown(EKeyInput::Q)) { Direction += -Up; }
		if (Input.IsKeyDown(EKeyInput::E)) { Direction += Up; }
		Direction.Normalize();
		RelativeLocation += Direction * CameraSpeed * DT;

		/**
		* @brief 마우스 위치 변화량을 감지하여 카메라의 회전을 담당합니다.
		*/
		const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
		RelativeRotation.X += MouseDelta.Y * KeySensitivityDegPerPixel;
		RelativeRotation.Y += MouseDelta.X * KeySensitivityDegPerPixel;

		// Pitch 클램프(짐벌 플립 방지)
		if (RelativeRotation.X > 89.0f) RelativeRotation.X = 89.0f;
		if (RelativeRotation.X < -89.0f) RelativeRotation.X = -89.0f;

		// Yaw 래핑(값이 무한히 커지지 않도록)
		if (RelativeRotation.Y > 180.0f)  RelativeRotation.Y -= 360.0f;
		if (RelativeRotation.Y < -180.0f) RelativeRotation.Y += 360.0f;
	}

	switch (CameraType)
	{
	case ECameraType::ECT_Perspective:
		UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		UpdateMatrixByOrth();
		break;
	}

	// TEST CODE 
	URenderer::GetInstance().UpdateConstant(ViewProjConstants);
}

void Camera::UpdateMatrixByPers()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix::RotationMatrixInverse(FVector::GetDegreeToRadian(RelativeRotation));
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

void Camera::UpdateMatrixByOrth()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix::RotationMatrixInverse(FVector::GetDegreeToRadian(RelativeRotation));
	ViewProjConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 */
	OrthoWidth = 2.0f * std::tanf(FVector::GetDegreeToRadian(FovY) * 0.5f);
	const float OrthoHeight = OrthoWidth / Aspect;
	const float Left = -OrthoWidth * 0.5f;
	const float Right = OrthoWidth * 0.5f;
	const float Bottom = -OrthoHeight * 0.5f;
	const float Top = OrthoHeight * 0.5f;

	FMatrix P = FMatrix::Identity();
	P.Data[0][0] = 2.0f / (Right - Left);
	P.Data[1][1] = 2.0f / (Top - Bottom);
	P.Data[2][2] = 1.0f / (FarZ - NearZ);
	P.Data[3][0] = -(Right + Left) / (Right - Left);
	P.Data[3][1] = -(Top + Bottom) / (Top - Bottom);
	P.Data[3][2] = -NearZ / (FarZ - NearZ);
	P.Data[3][3] = 1.0f;
	ViewProjConstants.Projection = P;
}

const FViewProjConstants Camera::GetFViewProjConstantsInverse() const
{
	/*
	* @brief View^(-1) = R * T
	*/
	FViewProjConstants Result = {};
	FMatrix R = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
	FMatrix T = FMatrix::TranslationMatrix(RelativeLocation);
	Result.View = R * T;

	if (CameraType == ECameraType::ECT_Orthographic)
	{
		const float OrthoHeight = OrthoWidth / Aspect;
		const float Left = -OrthoWidth * 0.5f;
		const float Right = OrthoWidth * 0.5f;
		const float Bottom = -OrthoHeight * 0.5f;
		const float Top = OrthoHeight * 0.5f;

		FMatrix P = FMatrix::Identity();
		// A^{-1} (대각)
		P.Data[0][0] = (Right - Left) * 0.5f;  // (r-l)/2
		P.Data[1][1] = (Top - Bottom) * 0.5f; // (t-b)/2
		P.Data[2][2] = (FarZ - NearZ);               // (zf-zn)
		// -b A^{-1} (마지막 행의 x,y,z)
		P.Data[3][0] = (Right + Left) * 0.5f;   // (r+l)/2
		P.Data[3][1] = (Top + Bottom) * 0.5f; // (t+b)/2
		P.Data[3][2] = NearZ;                      // zn
		P.Data[3][3] = 1.0f;
		Result.Projection = P;
	}
	else if ((CameraType == ECameraType::ECT_Perspective))
	{
		const float FovRadian = FVector::GetDegreeToRadian(FovY);
		const float F = 1.0f / std::tanf(FovRadian * 0.5f);
		FMatrix P = FMatrix::Identity();
		// | aspect/F   0      0         0 |
		// |    0      1/F     0         0 |
		// |    0       0      0   -(zf-zn)/(zn*zf) |
		// |    0       0      1        zf/(zn*zf)  |
		P.Data[0][0] = Aspect / F;
		P.Data[1][1] = 1.0f / F;
		P.Data[2][2] = 0.0f;
		P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
		P.Data[3][2] = 1.0f;
		P.Data[3][3] = FarZ / (NearZ * FarZ);
		Result.Projection = P;
	}

	return Result;
}
