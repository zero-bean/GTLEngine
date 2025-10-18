#pragma once
#include "Actor.h"
#include "DirectionalLightComponent.h"

class UGizmoArrowComponent;

class UDirectionalLightComponent;

class ADirectionalLightActor : public AActor
{
public:
    DECLARE_SPAWNABLE_CLASS(ADirectionalLightActor, AActor, "Directional Light");
    ADirectionalLightActor();
    ~ADirectionalLightActor() override;

    void Tick(float DeltaTime) override;

    UGizmoArrowComponent* GetDirectionComponent() const { return DirectionComponent; }

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    UDirectionalLightComponent* DirectionalLightComponent = nullptr;
    UGizmoArrowComponent* DirectionComponent = nullptr;
};
