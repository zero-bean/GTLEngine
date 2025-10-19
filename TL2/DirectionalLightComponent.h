#pragma once
#include "LightComponent.h"

struct FComponentData;
struct FDirectionalLightProperty;

class UDirectionalLightComponent : public ULightComponent
{
public:
	DECLARE_SPAWNABLE_CLASS(UDirectionalLightComponent, ULightComponent, "Directional Light Component")
	UDirectionalLightComponent();
	~UDirectionalLightComponent() override;

	// Serialization for transform and properties
	virtual void Serialize(bool bIsLoading, FComponentData& InOut) override;

	virtual void TickComponent(float DeltaSeconds) override;

	// Direction은 Scene Comp의 회전방향
	FVector GetDirection() const { return this->GetWorldRotation().GetForwardVector(); }
	int32 IsEnabledSpecular() const { return bEnableSpecular; }
	void SetSpecularEnable(bool bEnable);

	// Editor Details
	void RenderDetails() override;

protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;
	
	int32 bEnableSpecular = 1;
};
