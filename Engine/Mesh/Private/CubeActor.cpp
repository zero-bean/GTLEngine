#include "pch.h"
#include "Mesh/Public/CubeActor.h"

ACubeActor::ACubeActor()
{
	CubeComponent = GetCubeComponent();
	RootComponent = CubeComponent;
}

UCubeComponent* ACubeActor::GetCubeComponent()
{
	return CreateDefaultSubobject<UCubeComponent>("CubeComponent");
}
