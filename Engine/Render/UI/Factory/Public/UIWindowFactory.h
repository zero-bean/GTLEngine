#pragma once

class UControlPanelWindow;
class UInputStatusWindow;
class UPerformanceWindow;
class ULevelManagerWindow;

/**
 * @brief UI 윈도우 도킹 방향
 */
enum class EUIDockDirection : uint8_t
{
	None, // 도킹 없음
	Left, // 왼쪽 도킹
	Right, // 오른쪽 도킹
	Top, // 상단 도킹
	Bottom, // 하단 도킹
	Center, // 중앙 도킹
};

/**
* @brief UI 윈도우들을 쉽게 생성하기 위한 팩토리 클래스
 */
class UUIWindowFactory
{
public:
	static void CreateDefaultUILayout();
	static UPerformanceWindow* CreatePerformanceWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UInputStatusWindow* CreateInputStatusWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UControlPanelWindow* CreateActorInspectorWindow(EUIDockDirection InDockDirection = EUIDockDirection::Left);
	static ULevelManagerWindow* CreateLevelIOWindow(EUIDockDirection InDockDirection = EUIDockDirection::Center);
};

