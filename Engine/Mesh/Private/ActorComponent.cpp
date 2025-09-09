#include "pch.h"
#include "Mesh/Public/ActorComponent.h"

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent()
{

}

void UActorComponent::EndPlay()
{

}
