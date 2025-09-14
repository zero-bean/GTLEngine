#pragma once

#include "ImGuiWindowWrapper.h"

class USceneManagerPanel : public ImGuiWindowWrapper
{
public:
	USceneManagerPanel(class USceneManager* InSceneManager, TFunction<void(class UPrimitiveComponent*)> InOnPrimitiveSelected);

	virtual void RenderContent() override;

private:
	class USceneManager* SceneManager;
	TFunction<void(class UPrimitiveComponent*)> OnPrimitiveSelected;
};