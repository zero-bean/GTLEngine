#include "pch.h"
#include "RotationMovementComponent.h"
#include "ImGui/imgui.h"

URotationMovementComponent::URotationMovementComponent()
	: RotationRate(FVector(0.f, 80.f, 0.f))
	, PivotTranslation(FVector(0.f, 0.f, 0.f))
	, bRotationInLocalSpace(true)
{
	bCanEverTick = true;
	WorldTickMode = EComponentWorldTickMode::PIEOnly; // PIE에서만 작동
}

void URotationMovementComponent::TickComponent(float DeltaSeconds)
{
	UActorComponent::TickComponent(DeltaSeconds);
	if (!UpdatedComponent)
	{
		return;
	}

	// Rotations Per Second
	const FVector DeltaRotation = RotationRate * DeltaSeconds;
	const FQuat DeltaQuat = FQuat::MakeFromEuler(DeltaRotation);

	// Apply Rotation
	if (PivotTranslation.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		if (bRotationInLocalSpace)
		{
			UpdatedComponent->AddLocalRotation(DeltaQuat);
		}
		else
		{
			UpdatedComponent->AddWorldRotation(DeltaQuat);
		}
		return;
	}

	// Pivot Translation
	// Get World Transform
	FTransform WorldTransform = UpdatedComponent->GetWorldTransform();

	// Calculate world position of pivot
	FVector WorldPivot = WorldTransform.TransformPosition(PivotTranslation);

	// Calculate new rotation after applying DeltaQuat
	FQuat CurrentRot = WorldTransform.Rotation;
	FQuat NewRot = (bRotationInLocalSpace)
		? (CurrentRot * DeltaQuat)	// local-axis rotation
		: (DeltaQuat * CurrentRot); // world-axis rotation

	// Compute new location rotated around the pivot point
	// P' = C + ΔQ * (P - C)
	FVector WorldLoc = WorldTransform.Translation;
	FVector RotatedLoc = WorldPivot + DeltaQuat.RotateVector(WorldLoc - WorldPivot);

	// Apply the new world-space location and rotation
	UpdatedComponent->SetWorldLocationAndRotation(RotatedLoc, NewRot);
}

UObject* URotationMovementComponent::Duplicate()
{
	URotationMovementComponent* NewComp = Cast<URotationMovementComponent>(UMovementComponent::Duplicate());
	if (NewComp)
	{
		NewComp->RotationRate = RotationRate;
		NewComp->PivotTranslation = PivotTranslation;
		NewComp->bRotationInLocalSpace = bRotationInLocalSpace;
	}
	return NewComp;
}

void URotationMovementComponent::DuplicateSubObjects()
{
	UMovementComponent::DuplicateSubObjects();
}

void URotationMovementComponent::RenderDetails()
{
	ImGui::Text("Rotation Movement Component Settings");
	ImGui::Separator();

	// Rotate in Local Space
	bool bLocalRotation = GetRotationInLocalSpace();
	if (ImGui::Checkbox("Rotate in Local Space", &bLocalRotation))
	{
		SetRotationInLocalSpace(bLocalRotation);
	}

	// Rotation Rate
	FVector RotationRate = GetRotationRate();
	if (ImGui::DragFloat3("Rotation Rate", &RotationRate.X, 0.1f))
	{
		SetRotationRate(RotationRate);
	}

	// Pivot Translation
	FVector PivotTranslation = GetPivotTranslation();
	if (ImGui::DragFloat3("Pivot Translation", &PivotTranslation.X, 0.1f))
	{
		SetPivotTranslation(PivotTranslation);
	}
}