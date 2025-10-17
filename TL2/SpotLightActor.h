#pragma once
#include "Actor.h"

class USpotLightComponent;

class ASpotLightActor : public AActor
{
public:
	DECLARE_SPAWNABLE_CLASS(ASpotLightActor, AActor, "Spot Light")

	ASpotLightActor();
	~ASpotLightActor() override;

	void Tick(float DeltaTime) override;

	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

protected:
	USpotLightComponent* SpotLightComponent;
};

