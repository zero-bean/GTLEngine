#include "pch.h"
#include "PointLightActor.h"

APointLightActor::APointLightActor()
{
}

APointLightActor::~APointLightActor()
{
}

void APointLightActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

UObject* APointLightActor::Duplicate()
{
    return AActor::Duplicate();
}

void APointLightActor::DuplicateSubObjects()
{
    AActor::DuplicateSubObjects();
}
