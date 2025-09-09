#pragma once

#include "Mesh/Public/Actor.h"

class ASquareActor : public AActor
{
	using Super = AActor;
public:
	ASquareActor();
	virtual ~ASquareActor() override {}

	USquareComponent* GetSquareComponent();

private:
	USquareComponent* SquareComponent = nullptr;
};
