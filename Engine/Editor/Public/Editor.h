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


	void Update(HWND WindowHandle);
	void RenderEditor();

private:
	UObjectPicker ObjectPicker;
	UCamera Camera;
	UGizmo Gizmo;
	UAxis Axis;
	UGrid Grid;
};
