#include "pch.h"
#include "Render/UI/Window/Public/ExperimentalFeatureWindow.h"

#include "Render/UI/Widget/Public/InputInformationWidget.h"

/**
 * @brief Window Constructor
 */
UExperimentalFeatureWindow::UExperimentalFeatureWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Experimental Feature";
	Config.DefaultSize = ImVec2(300, 400);
	Config.DefaultPosition = ImVec2(10, 220);
	Config.MinSize = ImVec2(250, 200);
	Config.DockDirection = EUIDockDirection::Right;
	Config.Priority = 5;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UInputInformationWidget);
}

/**
 * @brief Initializer
 */
void UExperimentalFeatureWindow::Initialize()
{
	UE_LOG("ExperimentalFeatureWindow: Successfully Initialized");
}
