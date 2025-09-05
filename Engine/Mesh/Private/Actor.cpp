#include "Mesh/Public/Actor.h"
#include "Mesh/Public/SceneComponent.h"

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		delete Component;
	}
	OwnedComponents.clear();
}


template<typename T>
T* AActor::CreateDefaultSubobject(const FString& Name)
{
	T* NewComponent = new T();

	NewComponent.Outer = this;

	NewComponent.Name = Name;

	OwnedComponents.push_back(NewComponent);

	return NewComponent;
}

void AActor::SetActorLocation(const FVector& Location)
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(Location);
	}
}

void AActor::SetActorRotation(const FVector& Rotation)
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(Rotation);
	}
}

void AActor::SetActorScale3D(const FVector& Scale)
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(Scale);
	}
}

const FVector& AActor::GetActorLocation()
{
	if (RootComponent)
	{
		return RootComponent->GetRelativeLocation();
	}
	return FVector{ 0,0,0 };
}

const FVector& AActor::GetActorRotation()
{
	if (RootComponent)
	{
		return RootComponent->GetRelativeRotation();
	}
	return FVector{ 0,0,0 };
}

const FVector& AActor::GetActorScale3D()
{
	if (RootComponent)
	{
		return RootComponent->GetRelativeScale3D();
	}
	return FVector{ 0,0,0 };
}


