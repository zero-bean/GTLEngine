#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Physics/Public/AABB.h"

UCLASS()
class UFireBallComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UFireBallComponent, UPrimitiveComponent)

public:
	UFireBallComponent();
	virtual ~UFireBallComponent();

	void TickComponent(float DeltaSeconds) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// Getter
	FLinearColor GetLinearColor() const;
	FVector4 GetColor() const { return Color.Color; }
	float GetIntensity() const { return Intensity; }
	float GetRadius() const { return Radius; }
	float GetRadiusFallOff() const { return RadiusFallOff; }

	// Setter
	void SetLinearColor(const FLinearColor& InLinearColor);
	void SetColor(const FVector4& InColor) { Color.Color = InColor; }
	void SetIntensity(const float& InIntensity) { Intensity = InIntensity; }
	void SetRadius(const float& InRadius);
	void SetRadiusFallOff(const float& InRadiusFallOff) { RadiusFallOff = InRadiusFallOff; }

private:
	FLinearColor Color{};
	float Intensity = 1.0f;
	float Radius = 10.0f;
	float RadiusFallOff = 0.1f;
};

