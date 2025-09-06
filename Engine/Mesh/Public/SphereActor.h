#pragma once

#include "Mesh/Public/Actor.h"


class ASphereActor : public AActor
{
public:
	ASphereActor();
	USphereComponent* GetSphereComponent();
private:
	USphereComponent* SphereComponent = nullptr;
};

