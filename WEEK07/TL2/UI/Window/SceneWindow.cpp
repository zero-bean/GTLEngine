#include "pch.h"
#include "SceneWindow.h"
#include "../Widget/SceneManagerWidget.h"
#include"RenderViewportSwitcherWidget.h"

//// UE_LOG 대체 매크로
//#define UE_LOG(fmt, ...)

USceneWindow::USceneWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Scene Manager";
	Config.DefaultSize = ImVec2(350, 500);
	Config.DefaultPosition = ImVec2(1225, 10);
	Config.MinSize = ImVec2(300, 400);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
}

void USceneWindow::Initialize()
{
	UE_LOG("SceneWindow: Successfully Initialized");
}
