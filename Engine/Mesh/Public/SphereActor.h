#pragma once

#include "Mesh/Public/Actor.h"


class ASphereActor : public AActor
{
public:
	ASphereActor();
private:
	USphereComponent* SphereComponent = nullptr;
};

