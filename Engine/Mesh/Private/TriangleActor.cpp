#include "pch.h"
#include "Mesh/Public/TriangleActor.h"

ATriangleActor::ATriangleActor()
{
	TriangleComponent = GetTriangleComponent();
	TriangleComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(TriangleComponent);
}

UTriangleComponent* ATriangleActor::GetTriangleComponent()
{
	return CreateDefaultSubobject<UTriangleComponent>("TriangleComponent");
}
