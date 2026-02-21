#pragma once

#include "Mesh/Public/Actor.h"

class ASquareActor : public AActor
{
	using Super = AActor;
public:
	ASquareActor();
	virtual ~ASquareActor() override {}

private:
	USquareComponent* SquareComponent = nullptr;
};
