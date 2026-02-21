#pragma once
#include "PrimitiveComponent.h"
#include "Component/Public/PrimitiveComponent.h"
// jft : change to scenecomponent inheritance and make 전용 render pass
UCLASS()
class ULightComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(ULightComponent, UPrimitiveComponent)

public:
	ULightComponent();

	void UpdateBrightness() {}
	void UpdateLightColor(FVector4 InColor);

	FVector4 GetLightColor() { return LightColor; }

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	FVector4 Brightness;
	FVector4 LightColor;
};
