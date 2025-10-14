#include "pch.h"
#include "Component/Movement/Public/MovementComponent.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"

IMPLEMENT_CLASS(UMovementComponent, UActorComponent)

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	UE_LOG("set updated component");
	UpdatedComponent = NewUpdatedComponent;
	UpdatedPrimitive = Cast<UPrimitiveComponent>(UpdatedComponent);
}

void UMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    // If not explicitly set in the editor, default to the owner's root component
    if (!UpdatedComponent)
    {
        if (AActor* Owner = GetOwner())
        {
            SetUpdatedComponent(Owner->GetRootComponent());
        }
    }
}

void UMovementComponent::UpdateComponentVelocity()
{
	if ( UpdatedComponent )
	{
		UpdatedComponent->ComponentVelocity = Velocity;
	}
}

void UMovementComponent::MoveUpdatedComponent( const FVector& Delta, const FQuaternion& NewRotation)
{
	if (UpdatedComponent)
	{
		return UpdatedComponent->MoveComponent(Delta, NewRotation);
	}
}
