#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/CubeComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	auto CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	SetRootComponent(CubeComponent);
}
