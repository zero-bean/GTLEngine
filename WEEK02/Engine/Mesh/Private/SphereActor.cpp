#include "pch.h"
#include "Mesh/Public/SphereActor.h"

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetOwner(this);
	SetRootComponent(SphereComponent);
}
