#pragma once
#include"LightComponent.h"

class ULocalLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(ULocalLightComponent, ULightComponent)
	ULocalLightComponent();
	~ULocalLightComponent() override;

	// Serialization for transform

};

