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

	void ProcessMouseInput(ULevel* InLevel, UGizmo& InGizmo);
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	UCamera Camera;
	UObjectPicker ObjectPicker;
	
	UGizmo Gizmo;
	UAxis Axis;
	UGrid Grid;
};
