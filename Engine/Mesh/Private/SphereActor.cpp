#include "pch.h"
#include "Mesh/Public/SphereActor.h"

ASphereActor::ASphereActor()
{
	SphereComponent = GetSphereComponent();
	SetRootComponent(SphereComponent);
}

USphereComponent* ASphereActor::GetSphereComponent()
{
	return CreateDefaultSubobject<USphereComponent>("SphereComponent");
}
