#pragma once
#include "PrimitiveComponent.h"

UCLASS()
class UHeightFogComponent :  public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UHeightFogComponent, UPrimitiveComponent)

public:
	UHeightFogComponent();
	virtual ~UHeightFogComponent() = default;

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const { return bEnabled; }

	void SetFogDensity(float InDensity);
	float GetFogDensity() const { return FogDensity; }

	void SetFogHeightFalloff(float InFalloff);
	float GetFogHeightFalloff() const { return FogHeightFalloff; }

	void SetStartDistance(float InDistance);
	float GetStartDistance() const { return StartDistance; }

	void SetFogCutoffDistance(float InDistance);
	float GetFogCutoffDistance() const { return FogCutoffDistance; }

	void SetFogMaxOpacity(float InOpacity);
	float GetFogMaxOpacity() const { return FogMaxOpacity; }

	void SetFogHeight(float InHeight);
	float GetFogHeight() const { return FogHeight; }

	void SetFogInscatteringColor(const FVector4& InColor);
	const FVector4& GetFogInscatteringColor() const { return FogInscatteringColor; }

	FHeightFogConstants BuildFogConstants() const;

private:
	bool bEnabled = true;
	float FogDensity = 0.05f;
	float FogHeightFalloff = 1.0f;
	float StartDistance = 0.0f;
	float FogCutoffDistance = 100.0f;
	float FogMaxOpacity = 1.0f;
	float FogHeight = 50.0f;
	FVector4 FogInscatteringColor = FVector4(0.0f, 1.0f, 1.0f, 1.0f);
};
