#pragma once
#include "Actor.h"
#include "BillboardComponent.h"
#include "DecalSpotLightComponent.h"

class ADecalSpotLightActor : public AActor
{
public:
	DECLARE_CLASS(ADecalSpotLightActor, AActor)

	ADecalSpotLightActor();
	virtual ~ADecalSpotLightActor() override;

	void Tick(float DeltaTime) override;

	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

	UDecalSpotLightComponent* GetDecalSpotLightComponent() const { return DecalSpotLightComponent; }
	UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }

protected:
	UBillboardComponent* SpriteComponent;
	UDecalSpotLightComponent* DecalSpotLightComponent;
};
