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

	/**
	 * @brief 이 FireBall의 빛이 특정 프리미티브에 도달하기 전에 다른 물체에 의해 가려지는지 확인합니다.
	 * @param InReceiver 빛을 받을 대상 프리미티브입니다.
	 * @param OutOccluder 빛을 가로막는 물체(장애물)입니다. 가려지지 않았다면 nullptr이 됩니다.
	 * @return 빛이 가려졌다면 true, 아니라면 false를 반환합니다.
	 */
	bool IsOccluded(const UPrimitiveComponent* InReceiver, UPrimitiveComponent*& OutOccluder) const;
	bool IsShadowsEnabled() const { return bEnableShadows; }

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
	void SetShadowsEnabled(bool bEnabled) { bEnableShadows = bEnabled; }

private:
	FLinearColor Color{};
	float Intensity = 1.0f;
	float Radius = 10.0f;
	float RadiusFallOff = 0.1f;
	bool bEnableShadows = false;
};

