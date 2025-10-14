#include "pch.h"
#include "Component/Movement/Public/MovementComponent.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Utility/Public/JsonSerializer.h"

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

void UMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadVector(InOutHandle, "Velocity", Velocity, FVector::ZeroVector());

        FString UpdatedName;
        if (FJsonSerializer::ReadString(InOutHandle, "UpdatedComponentName", UpdatedName))
        {
            if (AActor* Owner = GetOwner())
            {
                for (const auto& Comp : Owner->GetOwnedComponents())
                {
                    if (Comp && Comp->GetName().ToString() == UpdatedName)
                    {
                        if (USceneComponent* SC = Cast<USceneComponent>(Comp.Get()))
                        {
                            SetUpdatedComponent(SC);
                        }
                        break;
                    }
                }
            }
        }
    }
    else
    {
        InOutHandle["Velocity"] = FJsonSerializer::VectorToJson(Velocity);
        if (UpdatedComponent)
        {
            InOutHandle["UpdatedComponentName"] = UpdatedComponent->GetName().ToString();
        }
    }
}

UObject* UMovementComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
    auto Dup = static_cast<UMovementComponent*>(Super::Duplicate(Parameters));

    Dup->Velocity = Velocity;

    // Remap UpdatedComponent through duplication seed if available
    if (UpdatedComponent)
    {
        if (auto It = Parameters.DuplicationSeed.find(UpdatedComponent); It != Parameters.DuplicationSeed.end())
        {
            Dup->UpdatedComponent = static_cast<USceneComponent*>(It->second);
        }
        else
        {
            Dup->UpdatedComponent = UpdatedComponent;
        }
    }
    else
    {
        Dup->UpdatedComponent = nullptr;
    }

    Dup->UpdatedPrimitive = Cast<UPrimitiveComponent>(Dup->UpdatedComponent.Get());

    return Dup;
}
