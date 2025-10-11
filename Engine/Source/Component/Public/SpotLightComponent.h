#pragma once
#include "Component/Public/LightComponent.h"

UCLASS()
class USpotLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USpotLightComponent, ULightComponent)
public:
	USpotLightComponent();
	virtual ~USpotLightComponent() = default;

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

	FMatrix GetLightWorldMatrix(); // scale 값은 영향 안 미친다.
	FMatrix GetLightInverseWorldMatrix();
};
