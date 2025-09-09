#include "pch.h"
#include "Render/UI/Factory/Public/UIWindowFactory.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"
#include "Render/UI/Window/Public/InputStatusWindow.h"
#include "Render/UI/Window/Public/PerformanceWindow.h"
#include "Render/UI/Window/Public/LevelManangerWindow.h"
#include "Render/UI/Window/Public/CameraPanelWindow.h"

UConsoleWindow* UUIWindowFactory::CreateConsoleWindow(EUIDockDirection InDockDirection)
{
	auto& Window = UConsoleWindow::GetInstance();
	Window.GetMutableConfig().DockDirection = InDockDirection;
	return &Window;
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

UControlPanelWindow* UUIWindowFactory::CreateActorInspectorWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UControlPanelWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

ULevelManagerWindow* UUIWindowFactory::CreateLevelIOWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new ULevelManagerWindow();
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
	UIManager.RegisterUIWindow(CreatePerformanceWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateInputStatusWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateActorInspectorWindow(EUIDockDirection::Left));
	UIManager.RegisterUIWindow(CreateLevelIOWindow(EUIDockDirection::Center));
	UIManager.RegisterUIWindow(CreateCameraPanelWindow(EUIDockDirection::Left));
	cout << "[UIWindowFactory] Default UI Layout Created" << "\n";
}
