#include "pch.h"
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

//테스트용 렌더링 코드
void AActor::Render(const URenderer& Renderer)
{
	if (RootComponent)
	{
		Renderer.UpdateConstant(RootComponent->GetRelativeLocation(), RootComponent->GetRelativeRotation(), RootComponent->GetRelativeScale3D());
		for (auto& Components : OwnedComponents)
		{
			Components->Render(Renderer);
		}
	}
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

FVector& AActor::GetActorLocation()
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

FVector& AActor::GetActorRotation()
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

FVector& AActor::GetActorScale3D()
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}


