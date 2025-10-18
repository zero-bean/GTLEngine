#pragma once
#include"SceneComponent.h"
#include "SceneLoader.h"
class ULightComponent : public USceneComponent
{
public:
	DECLARE_SPAWNABLE_CLASS(ULightComponent, USceneComponent, "Light Component")
	ULightComponent();
	~ULightComponent() override;

	// Serialization for transform
	virtual void Serialize(bool bIsLoading, FComponentData& InOut);

	// Light common properties
	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	FLinearColor GetColor() const { return Color; }
	void SetColor(const FLinearColor& InColor) { Color = InColor; }

protected:
	float Intensity = 1.0f;
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};

