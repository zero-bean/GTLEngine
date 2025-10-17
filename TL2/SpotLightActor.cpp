#include "pch.h"
#include "SpotLightActor.h"
#include "SpotLightComponent.h"

ASpotLightActor::ASpotLightActor()
{
	Name = "Spot Light Actor";
	SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(FName("SpotLightComponent"));
}

ASpotLightActor::~ASpotLightActor()
{
}

void ASpotLightActor::Tick(float DeltaTime)
{
}

UObject* ASpotLightActor::Duplicate()
{
	return nullptr;
}

void ASpotLightActor::DuplicateSubObjects()
{
}
