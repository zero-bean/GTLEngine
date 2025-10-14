#include "pch.h"
#include "Component/Movement/Public/RotatingMovementComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include "Global/Matrix.h"

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

	// Compute new rotation using engine's matrix convention (Rx*Ry*Rz) and pivot delta
	const FVector OldEulerDeg = UpdatedComponent->GetRelativeRotation();
	const FVector NewEulerDeg = OldEulerDeg + (RotationRate * DeltaSeconds);

	const FVector OldEulerRad = FVector::GetDegreeToRadian(OldEulerDeg);
	const FVector NewEulerRad = FVector::GetDegreeToRadian(NewEulerDeg);

	const FMatrix Rold = FMatrix::RotationMatrix(OldEulerRad);
	const FMatrix Rnew = FMatrix::RotationMatrix(NewEulerRad);

	const FVector OldPivot = FMatrix::VectorMultiply(PivotTranslation, Rold);
	const FVector NewPivot = FMatrix::VectorMultiply(PivotTranslation, Rnew);
	const FVector DeltaLocation = (NewPivot - OldPivot);

	UpdatedComponent->SetRelativeLocation(UpdatedComponent->GetRelativeLocation() + DeltaLocation);
	UpdatedComponent->SetRelativeRotation(NewEulerDeg);
}

void URotatingMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadVector(InOutHandle, "RotationRate", RotationRate, FVector::ZeroVector());
        FJsonSerializer::ReadVector(InOutHandle, "PivotTranslation", PivotTranslation, FVector::ZeroVector());
    }
    else
    {
        InOutHandle["RotationRate"] = FJsonSerializer::VectorToJson(RotationRate);
        InOutHandle["PivotTranslation"] = FJsonSerializer::VectorToJson(PivotTranslation);
    }
}

UObject* URotatingMovementComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
    auto Dup = static_cast<URotatingMovementComponent*>(Super::Duplicate(Parameters));
    Dup->RotationRate = RotationRate;
    Dup->PivotTranslation = PivotTranslation;
    return Dup;
}
