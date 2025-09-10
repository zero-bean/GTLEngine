#pragma once
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/public/Axis.h"
#include "Core/Public/Object.h"
#include "Editor/Public/ObjectPicker.h"

class UEditor : public UObject
{
public:

	UEditor();
	~UEditor();

	
	void Update();
	void RenderEditor();

private:

	void ProcessMouseInput(ULevel* InLevel);
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	FVector GetGizmoDragLocation(FRay& WorldRay);
	FVector GetGizmoDragRotation(FRay& WorldRay);
	FVector GetGizmoDragScale(FRay& WorldRay);

	UCamera Camera;
	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	UGizmo Gizmo;
	UAxis Axis;
	UGrid Grid;
};
