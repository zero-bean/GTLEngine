#include "pch.h"
#include "Actor/Public/SphereActor.h"
#include "Component/Mesh/Public/SphereComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	auto SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SetRootComponent(SphereComponent);
}

