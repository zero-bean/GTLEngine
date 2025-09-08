#include "pch.h"
#include "Mesh/Public/CubeActor.h"

ACubeActor::ACubeActor()
{
	CubeComponent = GetCubeComponent();
	SetRootComponent(CubeComponent);
}

UCubeComponent* ACubeActor::GetCubeComponent()
{
	return CreateDefaultSubobject<UCubeComponent>("CubeComponent");
}
