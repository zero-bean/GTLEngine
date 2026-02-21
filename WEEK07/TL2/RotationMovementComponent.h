#pragma once
#include "MovementComponent.h"

class URotationMovementComponent : public UMovementComponent
{
public:
	DECLARE_CLASS(URotationMovementComponent, UMovementComponent)
	URotationMovementComponent();

	void TickComponent(float DeltaSeconds) override;

	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

	// Editor Details
	void RenderDetails() override;

	// Getter/Setter Func
	FVector GetRotationRate() const { return RotationRate; }
	FVector GetPivotTranslation() const { return PivotTranslation; }
	bool GetRotationInLocalSpace() const { return bRotationInLocalSpace; }
	void SetRotationRate(const FVector& InRotationRate) { RotationRate = InRotationRate; }
	void SetPivotTranslation(const FVector& InPivotTranslation) { PivotTranslation = InPivotTranslation; }
	void SetRotationInLocalSpace(bool InRotationInLocalSpace) { bRotationInLocalSpace = InRotationInLocalSpace; }

private:
	FVector RotationRate;
	FVector PivotTranslation;
	bool bRotationInLocalSpace;
};