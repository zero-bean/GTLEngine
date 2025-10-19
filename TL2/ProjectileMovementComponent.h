#pragma once
#include "MovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
public:
	DECLARE_CLASS(UProjectileMovementComponent, UMovementComponent)
	UProjectileMovementComponent();

	void TickComponent(float DeltaSeconds) override;

	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

	// Editor Details
	void RenderDetails() override;

	// Getter/Setter Func
	float GetInitialSpeed() const { return InitialSpeed; }
	float GetMaxSpeed() const { return MaxSpeed; }
	float GetGravityScale() const { return GravityScale; }
	void SetInitialSpeed(float InInitialSpeed) { InitialSpeed = InInitialSpeed; }
	void SetMaxSpeed(float InMaxSpeed) { MaxSpeed = InMaxSpeed; }
	void SetGravityScale(float InGravityScale) { GravityScale = InGravityScale; }

private:
	float InitialSpeed;
	float MaxSpeed;
	float GravityScale;
};
