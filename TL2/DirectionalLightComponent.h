#pragma once
#include "LightComponent.h"

struct FComponentData;
struct FDirectionalLightProperty;

class UDirectionalLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)
	UDirectionalLightComponent();
	~UDirectionalLightComponent() override;

	// Serialization for transform and properties
	virtual void Serialize(bool bIsLoading, FComponentData& InOut) override;

	virtual void TickComponent(float DeltaSeconds) override;

	FDirectionalLightProperty DirectionalData;

protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;
};
