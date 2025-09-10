#include "pch.h"
#include "Render/UI/Factory/Public/UIWindowFactory.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"
#include "Render/UI/Window/Public/InputStatusWindow.h"
#include "Render/UI/Window/Public/PerformanceWindow.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"

UConsoleWindow* UUIWindowFactory::CreateConsoleWindow(EUIDockDirection InDockDirection)
{
	auto& Window = UConsoleWindow::GetInstance();
	Window.GetMutableConfig().DockDirection = InDockDirection;
	return &Window;
}

UControlPanelWindow* UUIWindowFactory::CreateControlPanelWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UControlPanelWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UOutlinerWindow* UUIWindowFactory::CreateOutlinerWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UOutlinerWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UPerformanceWindow* UUIWindowFactory::CreatePerformanceWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UPerformanceWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UInputStatusWindow* UUIWindowFactory::CreateInputStatusWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UInputStatusWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UCameraPanelWindow* UUIWindowFactory::CreateCameraPanelWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UCameraPanelWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

void UUIWindowFactory::CreateDefaultUILayout()
{
	auto& UIManager = UUIManager::GetInstance();

	// 기본 레이아웃 생성
	UIManager.RegisterUIWindow(CreateConsoleWindow(EUIDockDirection::Bottom));
	UIManager.RegisterUIWindow(CreateControlPanelWindow(EUIDockDirection::Left));
	UIManager.RegisterUIWindow(CreateOutlinerWindow(EUIDockDirection::Center));
	// UIManager.RegisterUIWindow(CreatePerformanceWindow(EUIDockDirection::Right));
	// UIManager.RegisterUIWindow(CreateInputStatusWindow(EUIDockDirection::Right));
	// UIManager.RegisterUIWindow(CreateCameraPanelWindow(EUIDockDirection::Left));
	UE_LOG("UIWindowFactory: Default UI Layout Created Successfully");
}
