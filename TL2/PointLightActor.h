#pragma once
#include "Actor.h"

class UPointLightComponent;

class APointLightActor : public AActor
{
public:
    DECLARE_SPAWNABLE_CLASS(APointLightActor, AActor, "Point Light");

    APointLightActor();
    ~APointLightActor() override;

    void Tick(float DeltaTime) override;

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    UPointLightComponent* PointLightComponent;
    
};
