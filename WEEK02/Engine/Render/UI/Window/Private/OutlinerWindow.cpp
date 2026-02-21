#include "pch.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"

#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner";
	Config.DefaultSize = ImVec2(270, 340);
	Config.DefaultPosition = ImVec2(1305, 10);
	Config.MinSize = ImVec2(270, 50);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new UTargetActorTransformWidget);
	AddWidget(new UActorTerminationWidget);
}

void UOutlinerWindow::Initialize()
{
	UE_LOG("OutlinerWindow: Window가 성공적으로 생성되었습니다");
}
