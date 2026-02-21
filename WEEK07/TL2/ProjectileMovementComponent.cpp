#include "pch.h"
#include "ProjectileMovementComponent.h"
#include "ImGui/imgui.h"

UProjectileMovementComponent::UProjectileMovementComponent()
	: InitialSpeed(10.0f)
	, MaxSpeed(100.f)
	, GravityScale(1.0f)
{
	bCanEverTick = true;
	WorldTickMode = EComponentWorldTickMode::PIEOnly; // PIE에서만 작동
	// Initial speed (forward)
	Velocity = FVector(0.f, InitialSpeed, 0.0f);
}

void UProjectileMovementComponent::TickComponent(float DeltaSeconds)
{
	UMovementComponent::TickComponent(DeltaSeconds);
	Velocity.Z -= 9.8f * GravityScale * DeltaSeconds;

	if (Velocity.Size() > MaxSpeed)
	{
		Velocity = -Velocity.GetSafeNormal() * MaxSpeed;
	}

	FVector Delta = Velocity * DeltaSeconds;

	if (Delta.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		UpdatedComponent->AddWorldOffset(Delta);
	}
}

UObject* UProjectileMovementComponent::Duplicate()
{
	UProjectileMovementComponent* Newcomp = Cast<UProjectileMovementComponent>(UMovementComponent::Duplicate());
	if (Newcomp)
	{
		Newcomp->InitialSpeed = InitialSpeed;
		Newcomp->MaxSpeed = MaxSpeed;
		Newcomp->GravityScale = GravityScale;
	}
	return Newcomp;
}

void UProjectileMovementComponent::DuplicateSubObjects()
{
	UMovementComponent::DuplicateSubObjects();
}

void UProjectileMovementComponent::RenderDetails()
{
	ImGui::Text("Projectile Movement Component Settings");
	ImGui::Separator();

	// Initial Speed
	float InitialSpeed = GetInitialSpeed();
	if (ImGui::DragFloat("InitialSpeed", &InitialSpeed, 1.0f))
	{
		SetInitialSpeed(InitialSpeed);
	}
	// Max Speed
	float MaxSpeed = GetMaxSpeed();
	if (ImGui::DragFloat("MaxSpeed", &MaxSpeed, 1.0f))
	{
		SetMaxSpeed(MaxSpeed);
	}
	// Gravity Scale
	float GravityScale = GetGravityScale();
	if (ImGui::DragFloat("GravityScale", &GravityScale, 1.0f, 0.0f, 10000.0f))
	{
		SetGravityScale(GravityScale);
	}
}