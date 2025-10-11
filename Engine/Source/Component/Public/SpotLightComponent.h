#pragma once
#include "Component/Public/LightComponent.h"

UCLASS()
class USpotLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USpotLightComponent, ULightComponent)
public:
	USpotLightComponent();
	virtual ~USpotLightComponent();


};
