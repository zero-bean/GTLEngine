#pragma once
#include "LightComponent.h"

struct FComponentData;
struct FDirectionalLightProperty;
class UGizmoArrowComponent;

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
	FVector GetDirection();
	int32 IsEnabledSpecular() const { return bEnableSpecular; }
	void SetSpecularEnable(bool bEnable);

	UGizmoArrowComponent** GetDirectionComponent() { return &DirectionComponent; }

	// Editor Details
	void RenderDetails() override;

	void DrawDebugLines(class URenderer* Renderer, const FMatrix& View, const FMatrix& Proj) override;

protected:
	UObject* Duplicate() override;
	void DuplicateSubObjects() override;

private:
	int32 bEnableSpecular = 1;
	UGizmoArrowComponent* DirectionComponent = nullptr;
	FVector Direction;
};
