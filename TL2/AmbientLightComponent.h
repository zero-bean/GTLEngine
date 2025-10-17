#pragma once
#include"LightComponent.h"

struct FComponentData;
struct FAmbientLightProperty;

class UAmbientLightComponent : public ULightComponent
{
public:
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)
	UAmbientLightComponent();
	~UAmbientLightComponent() override;

	// Serialization for transform and properties
	virtual void Serialize(bool bIsLoading, FComponentData& InOut) override;

	virtual void TickComponent(float DeltaSeconds) override;

	FAmbientLightProperty AmbientData;

protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;
};

