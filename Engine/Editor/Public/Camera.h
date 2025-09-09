#pragma once
#include "Core/Public/Object.h"

class UCamera : public UObject
{
public:
	UCamera() :
		ViewProjConstants(FViewProjConstants()),
		Position(FVector(0, 0, -4.5f)), Rotation(FVector(0, 0, 0)),
		FovY(60.f), Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT),
		NearZ(0.1f), FarZ(100.f)
	{
	}
	~UCamera() {};

	void Update();
	void UpdateMatrix();

	void SetLocation(const FVector& InOtherPosition) { Position = InOtherPosition; }
	void SetRotation(const FVector& InOtherRotation) { Rotation = InOtherRotation; }
	void SetFovY(const float InOtherFovY) { FovY = InOtherFovY; }
	void SetAspect(const float InOtherAspect) { Aspect = InOtherAspect; }
	void SetNearZ(const float InOtherNearZ) { NearZ = InOtherNearZ; }
	void SetFarZ(const float InOtherFarZ) { FarZ = InOtherFarZ; }

	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	const FViewProjConstants GetFViewProjConstantsInverse() const;
	FVector& GetLocation() { return Position; }
	FVector& GetRotation() { return Rotation; }
	const FVector& GetForward() const { return Forward; }
	const float GetFovY() const { return FovY; }
	const float GetAspect() const { return Aspect; }
	const float GetNearZ() const { return NearZ; }
	const float GetFarZ() const { return FarZ; }

private:
	FViewProjConstants ViewProjConstants = {};
	FVector Position = {};
	FVector Rotation = {};
	FVector Forward = {0,0,1};
	FVector Up = {};
	FVector Right = {};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
};
