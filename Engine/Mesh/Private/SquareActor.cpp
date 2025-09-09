#include "pch.h"
#include "Mesh/Public/SquareActor.h"

ASquareActor::ASquareActor()
{
	SquareComponent = GetSquareComponent();
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(SquareComponent);
}

USquareComponent* ASquareActor::GetSquareComponent()
{
	return CreateDefaultSubobject<USquareComponent>("SquareComponent");
}
