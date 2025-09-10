#include "pch.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"

#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner Window";
	Config.DefaultSize = ImVec2(350, 280);
	Config.DefaultPosition = ImVec2(0, 700);
	Config.MinSize = ImVec2(350, 280);
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
	UE_LOG("OutlinerWindow: Successfully Initialized");
}
