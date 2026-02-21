#pragma once
#include"SceneComponent.h"
#include "SceneLoader.h"

class ULightComponent : public USceneComponent
{
public:
	DECLARE_SPAWNABLE_CLASS(ULightComponent, USceneComponent, "LightComponent")
	ULightComponent();
	~ULightComponent() override;

	// Serialization for transform
	virtual void Serialize(bool bIsLoading, FComponentData& InOut);
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

	virtual void DrawDebugLines(class URenderer* Renderer, const FMatrix& View, const FMatrix& Proj);

	// Light common properties
	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	float GetColorTemperature() const;
	void SetColorTemperature(float InTemperature);

	FLinearColor GetTintColor() const { return TintColor; }
	void SetTintColor(const FLinearColor& InColor);

	FLinearColor GetFinalColor() const { return FinalColor * Intensity; }

	bool IsEnabledDebugLine() const { return bEnableDebugLine; }
	void SetDebugLineEnable(bool bEnable) { bEnableDebugLine = bEnable; }

    void UpdateSpriteColor(const FLinearColor& InSpriteColor);

    // Global toggle for light debug line rendering
    static void SetGlobalShowLightDebugLines(bool bEnable) { GShowLightDebugLines = bEnable; }
    static bool IsGlobalShowLightDebugLines() { return GShowLightDebugLines; }

protected:
	void CopyLightProperties(ULightComponent* Source);

private:
    void UpdateFinalColor();

protected:
	float Intensity = 1.0f;
	float Temperature = 6500.0f;
	// 최종 색상 = Light Color * Temperature Color
	FLinearColor FinalColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// 원래 쓰던 색상 - Light Color
	FLinearColor TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// 색온도에 의해 결정된 색상
    FLinearColor TempColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    bool bEnableDebugLine = true;

    // Global show flag mirror (toggled via viewport UI)
    static bool GShowLightDebugLines;
};
