#include "pch.h"
#include "MovementComponent.h"

UMovementComponent::UMovementComponent()
	: UpdatedComponent(nullptr)
	, Velocity(FVector(0.f, 5.f, 0.f))
{
	bCanEverTick = true;
	WorldTickMode = EComponentWorldTickMode::PIEOnly; // PIE에서만 작동
}

void UMovementComponent::TickComponent(float DeltaSeconds)
{
	// Base class doesn't move anything.
	// Subclasses (Projectile, Rotation, etc.) implement their own behavior.
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* Component)
{
	UpdatedComponent = Component;
}

void UMovementComponent::OnRegister()
{
	if (UpdatedComponent == nullptr)
	{
		if (AActor* Owner = GetOwner())
		{
			SetUpdatedComponent(Owner->GetRootComponent());
		}
	}
}

UObject* UMovementComponent::Duplicate()
{
	UMovementComponent* NewComp = Cast<UMovementComponent>(Super_t::Duplicate());
	if (NewComp)
	{
		NewComp->Velocity = Velocity;
		NewComp->UpdatedComponent = nullptr;
	}
	return NewComp;
}

void UMovementComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}