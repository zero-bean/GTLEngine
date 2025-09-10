#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

class UConsoleWidget;

/**
 * @brief Console Window
 * Widget 기반으로 리팩토링된 콘솔 윈도우
 * 실제 콘솔 기능은 UConsoleWidget에서 처리
 */
class UConsoleWindow : public UUIWindow
{
public:
	void AddLog(const char* fmt, ...) const;
	void AddLog(ELogType InType, const char* fmt, ...) const;
	void AddSystemLog(const char* InText, bool bInIsError = false) const;

	static UConsoleWindow& GetInstance();
	void Initialize() override;

	// Console Direct Access
	UConsoleWidget* GetConsoleWidget() const { return ConsoleWidget; }

	// Special Member Function
	explicit UConsoleWindow(const FUIWindowConfig& InConfig = FUIWindowConfig());
	~UConsoleWindow() override;
	UConsoleWindow(const UConsoleWindow&) = delete;
	UConsoleWindow& operator=(const UConsoleWindow&) = delete;
	UConsoleWindow(UConsoleWindow&&) = delete;
	UConsoleWindow& operator=(UConsoleWindow&&) = delete;

	bool IsSingleton() override { return true; }

private:
	// Console Widget
	UConsoleWidget* ConsoleWidget;
};
