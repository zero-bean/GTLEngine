#pragma once
#include "Actor.h"

class USpotLightComponent;
class UBillboardComponent;

class ASpotLightActor : public AActor
{
public:
	DECLARE_CLASS(ASpotLightActor, AActor)

	ASpotLightActor();
	~ASpotLightActor() override;

	void Tick(float DeltaTime) override;

	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

protected:
    USpotLightComponent* SpotLightComponent;
    UBillboardComponent* SpotBillboard = nullptr;
};

