#include "pch.h"
#include "Mesh/Public/Actor.h"
#include "Mesh/Public/SceneComponent.h"

AActor::AActor() = default;

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}

	OwnedComponents.clear();
}

//테스트용 렌더링 코드
//void AActor::Render(const URenderer& Renderer)
//{
//	if (RootComponent)
//	{
//		Renderer.UpdateConstant(RootComponent->GetRelativeLocation(), RootComponent->GetRelativeRotation(), RootComponent->GetRelativeScale3D());
//		for (auto& Components : OwnedComponents)
//		{
//			Components->Render(Renderer);
//		}
//	}
//}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

void AActor::Tick()
{
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->TickComponent();
		}
	}
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}
