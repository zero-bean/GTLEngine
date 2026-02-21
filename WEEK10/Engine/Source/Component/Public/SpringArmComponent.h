#pragma once
#include "Component/Public/SceneComponent.h"

UCLASS()
class USpringArmComponent : public USceneComponent
{
	DECLARE_CLASS(USpringArmComponent, USceneComponent)

public:
	USpringArmComponent();
	virtual ~USpringArmComponent() = default;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;

	UObject* Duplicate() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;;

	// Settings
	void SetTargetArmLength(float NewLength) { TargetArmLength = NewLength; }
	float GetTargetArmLength() const { return TargetArmLength; }

	void SetEnableCameraLag(bool bEnable) { bEnableCameraLag = bEnable; }
	void SetCameraLagSpeed(float Speed) { CameraLagSpeed = Speed; }

	void SetEnableArmLengthLag(bool bEnable) { bEnableArmLengthLag = bEnable; }
	void SetArmLengthLagSpeed(float Speed) { ArmLengthLagSpeed = Speed; }

private:
	void UpdateCamera(float DeltaTime);

	// Settings
	float TargetArmLength;      // 기본 300.0f
	bool bEnableCameraLag;      // 기본 false
	float CameraLagSpeed;       // 기본 10.0f

	bool bEnableArmLengthLag;
	float ArmLengthLagSpeed;

	// Runtime
	FVector PreviousLocation;
	bool bIsFirstUpdate;
	FQuaternion LaggedWorldRotation;
	FVector LaggedWorldLocation;
	FVector PreviousChildRelativeLocation;
	float CurrentArmLength;
	bool bDoCollisionTest = true;
};
