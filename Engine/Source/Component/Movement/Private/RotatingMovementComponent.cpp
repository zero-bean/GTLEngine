#include "pch.h"
#include "Component/Movement/Public/RotatingMovementComponent.h"

IMPLEMENT_CLASS(URotatingMovementComponent, UMovementComponent)

void URotatingMovementComponent::TickComponent(float DeltaSeconds)
{
	if (!UpdatedComponent)
	{
		// Try to bind on first tick if not set via UI
		if (AActor* Owner = GetOwner())
		{
			SetUpdatedComponent(Owner->GetRootComponent());
		}
		if (!UpdatedComponent)
		{
			return; // nothing to update
		}
	}

	// Compute new rotation
	const FQuaternion OldRotation = FQuaternion::FromEuler(UpdatedComponent->GetRelativeRotation());
	const FQuaternion DeltaRotation = FQuaternion::FromEuler((RotationRate * DeltaSeconds));
	const FQuaternion NewRotation = OldRotation * DeltaRotation;

	const FVector OldPivot = OldRotation.RotateVector(PivotTranslation);
	const FVector NewPivot = NewRotation.RotateVector(PivotTranslation);
	FVector DeltaLocation = (OldPivot - NewPivot);

	MoveUpdatedComponent(DeltaLocation, NewRotation);
}
