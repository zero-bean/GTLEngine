#pragma once

#include "Mesh/Public/Actor.h"

class ATriangleActor : public AActor
{
	using Super = AActor;
public:
	ATriangleActor();
	virtual ~ATriangleActor() override {}

	UTriangleComponent* GetTriangleComponent();

private:
	UTriangleComponent* TriangleComponent = nullptr;
};
