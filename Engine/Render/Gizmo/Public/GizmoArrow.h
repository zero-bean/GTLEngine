#pragma once
#include "Mesh/Public/SceneComponent.h"

class UGizmoArrowComponent : public UPrimitiveComponent
{
public:
	UGizmoArrowComponent();
	void SetForward(FVector InForward) { Forward = InForward; }

	void OnClicked();
	void MoveActor(float Distance);
	void OnReleased();

	virtual void TickComponent() override;
private:
	FVector Forward = {0.f, 0.f, 0.f};
	bool bIsClicked = false;
};

