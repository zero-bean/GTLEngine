#include "pch.h"
#include "Render/UI/Factory/Public/UIWindowFactory.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/ActorInspectorWindow.h"
#include "Render/UI/Window/Public/InputStatusWindow.h"
#include "Render/UI/Window/Public/PerformanceWindow.h"

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

UActorInspectorWindow* UUIWindowFactory::CreateActorInspectorWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UActorInspectorWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

void UUIWindowFactory::CreateDefaultUILayout()
{
	auto& UIManager = UUIManager::GetInstance();

	// 기본 레이아웃 생성
	UIManager.RegisterUIWindow(CreatePerformanceWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateInputStatusWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateActorInspectorWindow(EUIDockDirection::Left));

	cout << "[UIWindowFactory] Default UI layout created" << endl;
}
