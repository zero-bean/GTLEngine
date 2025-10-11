#pragma once

#include "Actor/Public/Actor.h"

UCLASS()
class ALightActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ALightActor, AActor)
public:
	ALightActor();
	virtual ~ALightActor() = default;
};
