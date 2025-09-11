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
	Config.DefaultSize = ImVec2(350, 10);
	Config.DefaultPosition = ImVec2(420, 10);
	Config.MinSize = ImVec2(350, 10);
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
	UE_LOG("ExperimentalFeatureWindow: Window가 성공적으로 생성되었습니다");
}
