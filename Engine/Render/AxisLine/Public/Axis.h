#pragma once
#include "Mesh/Public/Actor.h"

class UAxisLineComponent;

class AAxis : public AActor
{
public:
	AAxis();
	~AAxis() override;

private:
	UAxisLineComponent* LineX = nullptr;
	UAxisLineComponent* LineY = nullptr;
	UAxisLineComponent* LineZ = nullptr;
};
