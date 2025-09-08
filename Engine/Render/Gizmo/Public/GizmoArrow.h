#pragma once
#include "Mesh/Public/SceneComponent.h"

class UGizmoArrowComponent : public UPrimitiveComponent
{
public:
	UGizmoArrowComponent();
	void SetForward(const FVector& InForward) { Forward = InForward; }
	void SetDefaultColor(const FVector4& InColor) { DefaultColor = InColor; }

	void OnClicked();
	void MoveActor(const FVector& Location);
	void OnReleased();

	virtual void TickComponent() override;
private:
	FVector Forward = {0.f, 0.f, 0.f};
	FVector DragStartLocation = {0.f, 0.f, 0.f};

	FVector4 DefaultColor = {1.f,1.f,1.f,1.f};

	bool bIsClicked = false;
};

