#include "pch.h"
#include "Mesh/Public/ActorComponent.h"

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{

}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent(float DeltaTime)
{

}

void UActorComponent::EndPlay()
{

}
