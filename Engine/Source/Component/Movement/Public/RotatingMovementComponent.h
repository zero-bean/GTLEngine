#pragma once
#include "MovementComponent.h"

UCLASS()

class URotatingMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(URotatingMovementComponent, UMovementComponent)

public:
	FVector PivotTranslation;
	FVector RotationRate;

	void TickComponent(float DeltaSeconds) override;
};
