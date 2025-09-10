#include "pch.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"

#include "Render/UI/Widget/Public/CameraControlWidget.h"
#include "Render/UI/Widget/Public/FPSWidget.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"
#include "Render/UI/Widget/Public/SceneIOWidget.h"

/**
 * @brief Control Panel Constructor
 * 적절한 사이즈의 윈도우 제공
 */
UControlPanelWindow::UControlPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Control Panel";
	Config.DefaultSize = ImVec2(400, 580);
	Config.DefaultPosition = ImVec2(0, 0);
	Config.MinSize = ImVec2(400, 200);
	Config.DockDirection = EUIDockDirection::Left;
	Config.Priority = 15;
	Config.bResizable = true;
	Config.bMovable = false;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UFPSWidget);
	AddWidget(new UPrimitiveSpawnWidget);
	AddWidget(new USceneIOWidget);
	AddWidget(new UCameraControlWidget);
}

/**
 * @brief 초기화 함수
 */
void UControlPanelWindow::Initialize()
{
	UE_LOG("ControlPanelWindow: Initialized");
}
