#pragma once
#include "MovementComponent.h"

UCLASS()

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UProjectileMovementComponent, UMovementComponent)

public:
	FVector ProjectileGravity;

	float ProjectileLifeSpan;
	float ProjectileDrag;
	float ProjectileMaxSpeed;
	float ProjectileMaxAngularSpeed;
	float ProjectileBounciness;

	void TickComponent(float DeltaSeconds) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
};
