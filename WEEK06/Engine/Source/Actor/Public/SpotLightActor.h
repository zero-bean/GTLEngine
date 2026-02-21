#pragma once

#include "Actor/Public/LightActor.h"

UCLASS()
class ASpotLightActor : public ALightActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASpotLightActor, ALightActor)
public:
	ASpotLightActor();
	virtual ~ASpotLightActor() = default;

	void BeginPlay() override;
};
