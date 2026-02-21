#pragma once
#include "Widget.h"

class ULightSensorComponent;

UCLASS()
class ULightSensorComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(ULightSensorComponentWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	ULightSensorComponentWidget() = default;
	~ULightSensorComponentWidget() override = default;

private:
	ULightSensorComponent* SensorComponent{};
};
