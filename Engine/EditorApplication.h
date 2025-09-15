#pragma once
#include "stdafx.h"
#include "UApplication.h"
#include "USphereComp.h"
#include "URaycastManager.h"
#include "UControlPanel.h"
#include "SceneManagerPanel.h"
#include "USceneComponentPropertyWindow.h"
#include "UBoundingBoxManager.h"

// Simple application that inherits from UApplication
class EditorApplication : public UApplication
{
private:
	UGizmoManager gizmoManager;
	TArray<USceneComponent*> sceneComponents;

	USceneComponent* selectedSceneComponent;

	UControlPanel* controlPanel;
	USceneManagerPanel* SceneManagerPanel;
	USceneComponentPropertyWindow* propertyWindow;

	UBoundingBoxManager AABBManager;

public:
	EditorApplication() = default;
	~EditorApplication()
	{
		delete controlPanel;
		delete propertyWindow;
		controlPanel = nullptr;
		propertyWindow = nullptr;
	}
	UScene* CreateDefaultScene() override;

	void OnSceneChange() override;

	UGizmoManager& GetGizmoManager() { return gizmoManager; }
	const UGizmoManager& GetGizmoManager() const { return gizmoManager; }
protected:
	void Update(float deltaTime) override;
	void Render() override;
	void RenderGUI() override;
	bool OnInitialize() override;
	void OnResize(int32 width, int32 height) override;

	void OnPrimitiveSelected(UPrimitiveComponent* Primitive);
};