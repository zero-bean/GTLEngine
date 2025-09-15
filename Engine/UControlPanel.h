#pragma once
#include "ImGuiWindowWrapper.h"
#include "USceneManager.h"
#include "USceneComponent.h"
#include "UGizmoManager.h"

class UShowFlagManager;

class UControlPanel : public ImGuiWindowWrapper
{
	USceneManager* SceneManager;
	UGizmoManager* GizmoManager;
	UShowFlagManager* ShowFlagManager;

	// Spawn Primitive Section
	TArray<UClass*> registeredTypes;
	TArray<FString> choiceStrList;
	TArray<const char*> choices;
	int32 primitiveChoiceIndex = 0;

	// Scene Management Section
	char sceneName[256] = "Default";

	// Camera Management Section


public:
	UControlPanel(USceneManager* sceneManager, UGizmoManager* gizmoManager, UShowFlagManager* InShowFlagManager);
	void RenderContent() override;
	void PrimaryInformationSection();
	void SpawnPrimitiveSection();
	void SceneManagementSection();
	void CameraManagementSection();
	void ViewModeSection();
	void ShowFlagSection();
	USceneComponent* CreateSceneComponentFromChoice(int index);
};

