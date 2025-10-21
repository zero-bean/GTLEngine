#pragma once
#include "Actor.h"
#include "DirectionalLightComponent.h"

class UDirectionalLightComponent;

class ADirectionalLightActor : public AActor
{
public:
    DECLARE_SPAWNABLE_CLASS(ADirectionalLightActor, AActor, "Directional Light");
    ADirectionalLightActor();
    ~ADirectionalLightActor() override;

    void Tick(float DeltaTime) override;    

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    UDirectionalLightComponent* DirectionalLightComponent = nullptr;    
};
