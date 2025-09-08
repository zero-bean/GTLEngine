#pragma once

#include "Mesh/Public/Actor.h"


class ACubeActor : public AActor
{
public:
	ACubeActor();
	UCubeComponent* GetCubeComponent();

private:
	UCubeComponent* CubeComponent = nullptr;
};
