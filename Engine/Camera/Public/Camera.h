#pragma once

class Camera
{
public:
	Camera() :
		ViewProjConstants(FViewProjConstants()),
		Position(FVector(0, 0, -4.5f)), Rotation(FVector(0, 0, 0)),
		FovY(60.f), Aspect(float(Render::INIT_SCREEN_HEIGHT) / Render::INIT_SCREEN_WIDTH),
		NearZ(0.1f), FarZ(100.f)
	{
	}
	~Camera() {};

	void Update();
	void UpdateMatrix();

	void SetLocation(const FVector& InOtherPosition) { Position = InOtherPosition; }
	void SetRotation(const FVector& InOtherRotation) { Rotation = InOtherRotation; }
	void SetFovY(const float InOtherFovY) { FovY = InOtherFovY; }
	void SetAspect(const float InOtherAspect) { Aspect = InOtherAspect; }
	void SetNearZ(const float InOtherNearZ) { NearZ = InOtherNearZ; }
	void SetFarZ(const float InOtherFarZ) { FarZ = InOtherFarZ; }

	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	FVector& GetLocation() { return Position; }
	FVector& GetRotation() { return Rotation; }
	const float GetFovY() const { return FovY; }
	const float GetAspect() const { return Aspect; }
	const float GetNearZ() const { return NearZ; }
	const float GetFarZ() const { return FarZ; }

private:
	FViewProjConstants ViewProjConstants = {};
	FVector Position = {};
	FVector Rotation = {};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
};
