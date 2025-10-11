#pragma once
#include "PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"

UCLASS()
class ULightComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(ULightComponent, USceneComponent)

public:
	ULightComponent();

	void UpdateBrightness();
	void UpdateLightColor();
};
