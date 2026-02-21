#pragma once

#include "Mesh/Public/Actor.h"


class ACubeActor : public AActor
{
public:
	ACubeActor();

private:
	UCubeComponent* CubeComponent = nullptr;
};
