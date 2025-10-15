#include "pch.h"
#include "Component/Movement/Public/ProjectileMovementComponent.h"

#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UProjectileMovementComponent, UMovementComponent)

void UProjectileMovementComponent::TickComponent(float DeltaSeconds)
{
	if (!UpdatedComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			SetUpdatedComponent(Owner->GetRootComponent());
			UpdateComponentVelocity();
		}
		if (!UpdatedComponent)
		{
			return; // nothing to update
		}
	}

	const FVector OldVelocity = Velocity;
	const FVector MoveDelta = Velocity * DeltaSeconds + ProjectileGravity * DeltaSeconds * DeltaSeconds * 0.5f;
	Velocity = OldVelocity + ProjectileGravity * DeltaSeconds;
	UpdateComponentVelocity();

	UpdatedComponent->SetRelativeLocation(UpdatedComponent->GetRelativeLocation() + MoveDelta);
}

void UProjectileMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "Velocity", Velocity, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "ProjectileGravity", ProjectileGravity, FVector::ZeroVector());
	}
	else
	{
		InOutHandle["Velocity"] = FJsonSerializer::VectorToJson(Velocity);
		InOutHandle["ProjectileGravity"] = FJsonSerializer::VectorToJson(ProjectileGravity);
	}
}

UObject* UProjectileMovementComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UProjectileMovementComponent*>(Super::Duplicate(Parameters));
	DupObject->Velocity = Velocity;
	DupObject->ProjectileGravity = ProjectileGravity;
	return DupObject;
}
