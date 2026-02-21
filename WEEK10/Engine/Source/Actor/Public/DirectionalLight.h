#pragma once
#include "Actor/Public/Light.h"
#include "Component/Public/DirectionalLightComponent.h"


UCLASS()
class ADirectionalLight : public ALight
{
    GENERATED_BODY()
    DECLARE_CLASS(ADirectionalLight, ALight)

public:
    ADirectionalLight();

public:
    virtual UClass* GetDefaultRootComponent() override;

    UDirectionalLightComponent* GetDirectionalLightComponent() const;

private:
    UDirectionalLightComponent* DirectionalLightComponent = nullptr;
};




