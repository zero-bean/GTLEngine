#pragma once

#include "Actor/Public/Actor.h"

UCLASS()
class ABillboardActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ABillboardActor, AActor)
public:
	ABillboardActor();
	virtual ~ABillboardActor() = default;

};
