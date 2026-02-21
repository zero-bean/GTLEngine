#include "pch.h"
#include "Mesh/Public/SquareActor.h"

ASquareActor::ASquareActor()
{
	SquareComponent = CreateDefaultSubobject<USquareComponent>("SquareComponent");
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SquareComponent->SetOwner(this);
	SetRootComponent(SquareComponent);
}
