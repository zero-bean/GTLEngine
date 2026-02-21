#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UCameraComponent;

UCLASS()
class UCameraComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraComponentWidget, UWidget)
public:
	void Initialize() override {}
	void Update() override;
	void RenderWidget() override;

private:
	UCameraComponent* CameraComponent = nullptr;
};
