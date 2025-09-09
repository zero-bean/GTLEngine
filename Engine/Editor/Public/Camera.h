#pragma once
#include "Core/Public/Object.h"

enum class ECameraType
{
	ECT_Orthographic,
	ECT_Perspective
};

class UCamera : public UObject
{
public:
	UCamera() :
		ViewProjConstants(FViewProjConstants()),
		RelativeLocation(FVector(-5.f, 10.f, -5.f)), RelativeRotation(FVector(45, 45, 0)),
		FovY(90.f), Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT),
		NearZ(0.1f), FarZ(100.f), CameraType(ECameraType::ECT_Perspective)
	{
	}
	~UCamera() {};

	void Update();
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
	void SetCameraType(const ECameraType InCameraType) { CameraType = InCameraType; }

	/**
	 * @brief Getter
	 */
	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	const FViewProjConstants GetFViewProjConstantsInverse() const;

	FRay ConvertToWorldRay(int PixelX, int PixelY, int ViewportW, int ViewportH) const;

	FVector& GetLocation() { return RelativeLocation; }
	FVector& GetRotation() { return RelativeRotation; }
	const FVector& GetForward() const { return Forward; }
	const FVector& GetUp() const { return Up; }
	const FVector& GetRight() const { return Right; }
	const float GetFovY() const { return FovY; }
	const float GetAspect() const { return Aspect; }
	const float GetNearZ() const { return NearZ; }
	const float GetFarZ() const { return FarZ; }
	const ECameraType GetCameraType() const { return CameraType; }

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
	FVector Forward = { 0,0,1 };
	FVector Up = {};
	FVector Right = {};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
	float OrthoWidth = {};
	ECameraType CameraType = {};
};
