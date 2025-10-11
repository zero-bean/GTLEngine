#pragma once
#include "Actor.h"

class UDecalComponent;

UCLASS()
class ADecalActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ADecalActor, AActor)

public:
	ADecalActor();
	virtual ~ADecalActor();

	UDecalComponent* GetDecalComponent() const;
};

