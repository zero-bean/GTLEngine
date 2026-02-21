#pragma once
#include "Core/Public/Object.h"
class UConfigManager;

struct FViewProperties;

enum class EViewportCameraType : uint8_t
{
	Perspective,
	Ortho_Top,
	Ortho_Bottom,
	Ortho_Front,
	Ortho_Back,
	Ortho_Left,
	Ortho_Right
};

// EViewportType을 문자열로 변환하는 헬퍼 함수
inline const char* ClientCameraTypeToString(EViewportCameraType InType)
{
	switch (InType)
	{
	case EViewportCameraType::Perspective:   return "Perspective";
	case EViewportCameraType::Ortho_Top:     return "Top";
	case EViewportCameraType::Ortho_Bottom:  return "Bottom";
	case EViewportCameraType::Ortho_Front:   return "Front";
	case EViewportCameraType::Ortho_Back:    return "Back";
	case EViewportCameraType::Ortho_Left:    return "Left";
	case EViewportCameraType::Ortho_Right:   return "Right";
	default:                                 return "Unknown";
	}
}

class UCamera : public UObject
{

	DECLARE_CLASS(UCamera, UObject)

public:
	// Camera Settings Constants
	static constexpr float MIN_SPEED = 10.0f;
	static constexpr float MAX_SPEED = 200.0f;
	static constexpr float DEFAULT_SPEED = 100.0f;
	static constexpr float SPEED_ADJUST_STEP = 2.5f;
	static constexpr float MIN_ORTHO_WHEEL_WIDTH = 1.0f;
	static constexpr float MAX_ORTHO_WHEEL_WIDTH = 5000.0f;

	UCamera();
	~UCamera() override;

	/* *
	* @brief Update 관련 함수
	* UpdateInput 함수는 사용자 입력으로 비롯된 변화의 갱신를 담당합니다.
	* Update, UpdateMatrix 함수들은 카메라의 변환 행렬의 갱신을 담당합니다.
	*/
	FVector UpdateInput();
	void Update(const D3D11_VIEWPORT& InViewport);
	void UpdateMatrixByPers();
	void UpdateMatrixByOrth();

	/**
	 * @brief Setter
	 */
	void SetLocation(const FVector& InOtherPosition) { RelativeLocation = InOtherPosition; }
	void SetRotation(const FVector& InOtherRotation) { RelativeRotation = InOtherRotation; }
	void SetFovY(const float InOtherFovY) { FovY = InOtherFovY; }
	void SetAspect(const float InOtherAspect) { Aspect = InOtherAspect; }
	void SetNearZ(const float InOtherNearZ) { NearZ = InOtherNearZ; }
	void SetFarZ(const float InOtherFarZ) { FarZ = InOtherFarZ; }
	void SetOrthoWidth(const float InOrthoWidth) { OrthoWidth = InOrthoWidth; }
	void SetCameraType(const EViewportCameraType InCameraType) { CameraType = InCameraType; }

	/**
	 * @brief Getter
	 */
	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	const FViewProjConstants GetFViewProjConstantsInverse() const;

	FRay ConvertToWorldRay(float NdcX, float NdcY) const;

	FVector CalculatePlaneNormal(const FVector4& Axis);
	FVector CalculatePlaneNormal(const FVector& Axis);
	FVector& GetLocation() { return RelativeLocation; }
	FVector& GetRotation() { return RelativeRotation; }
	const FVector& GetForward() const { return Forward; }
	const FVector& GetUp() const { return Up; }
	const FVector& GetRight() const { return Right; }
	float GetFovY() const { return FovY; }
	float GetAspect() const { return Aspect; }
	float GetNearZ() const { return NearZ; }
	float GetFarZ() const { return FarZ; }
	float GetOrthoWidth() const { return OrthoWidth; }
	EViewportCameraType GetCameraType() const { return CameraType; }

	// Camera Movement Speed Control
	float GetMoveSpeed() const { return CurrentMoveSpeed; }
	void SetMoveSpeed(float InSpeed)
	{
		CurrentMoveSpeed = clamp(InSpeed, MIN_SPEED, MAX_SPEED);
		// CurrentMoveSpeed = min(InSpeed, MAX_SPEED);
	}
	void AdjustMoveSpeed(float InDelta) { SetMoveSpeed(CurrentMoveSpeed + InDelta); }

	/* *
	 * @brief 행렬 형태로 저장된 좌표와 변환 행렬과의 연산한 결과를 반환합니다.
	 */
	inline FVector4 MultiplyPointWithMatrix(const FVector4& Point, const FMatrix& Matrix) const
	{
		FVector4 Result = Point * Matrix;
		/* *
		 * @brief 좌표가 왜곡된 공간에 남는 것을 방지합니다.
		 */
		if (Result.W != 0.f) { Result *= (1.f / Result.W); }

		return Result;
	}

private:
	FViewProjConstants ViewProjConstants = {};
	FVector RelativeLocation = {};
	FVector RelativeRotation = {};
	FVector Forward = { 1,0,0 };
	FVector Up = {0,0,1};
	FVector Right = {0,1,0};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
	float OrthoWidth = {};
	EViewportCameraType CameraType = EViewportCameraType::Perspective;

	// Dynamic Movement Speed
	float CurrentMoveSpeed = DEFAULT_SPEED;
};
