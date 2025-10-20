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

	virtual void DrawDebugLines(class URenderer* Renderer);

	// Light common properties
	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	float GetColorTemperature() const;
	void SetColorTemperature(float InTemperature);

	FLinearColor GetTintColor() const { return TintColor; }
	void SetTintColor(const FLinearColor& InColor);

	FLinearColor GetFinalColor() const { return FinalColor; }

private:
	void UpdateFinalColor();

protected:
	float Intensity = 1.0f;
	float Temperature = 1000.0f;
	// 최종 색상 = Light Color * Temperature Color
	FLinearColor FinalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// 원래 쓰던 색상 - Light Color
	FLinearColor TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// 색온도에 의해 결정된 색상
	FLinearColor TempColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};