#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
}

UClass* AStaticMeshActor::GetDefaultRootComponent()
{
	return UStaticMeshComponent::StaticClass();
}

UStaticMeshComponent* AStaticMeshActor::GetStaticMeshComponent() const
{
	return Cast<UStaticMeshComponent>(GetRootComponent());
}
