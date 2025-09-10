#include "pch.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Widget/Public/ConsoleWidget.h"

IMPLEMENT_SINGLETON(UConsoleWindow)

UConsoleWindow::~UConsoleWindow()
{
	if (ConsoleWidget)
	{
		ConsoleWidget->CleanupSystemRedirect();
		ConsoleWidget->ClearLog();
	}
}

UConsoleWindow::UConsoleWindow(const FUIWindowConfig& InConfig)
	: UUIWindow(InConfig)
{
	// 콘솔 윈도우 기본 설정
	FUIWindowConfig Config = InConfig;
	Config.WindowTitle = "Game Console";
	Config.DefaultSize = ImVec2(520, 600);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.MinSize = ImVec2(400, 300);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	SetConfig(Config);

	ConsoleWidget = new UConsoleWidget;
	AddWidget(ConsoleWidget);
}

void UConsoleWindow::Initialize()
{
	AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
	AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");

	// Initialize System Output Redirection
	ConsoleWidget->InitializeSystemRedirect();
}

/**
 * @brief 외부에서 ConsoleWidget에 접근할 수 있도록 하는 래핑 함수
 */
void UConsoleWindow::AddLog(const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(buf);
	}
}

void UConsoleWindow::AddLog(ELogType InType, const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(InType, buf);
	}
}

void UConsoleWindow::AddSystemLog(const char* InText, bool bInIsError) const
{
	if (ConsoleWidget)
	{
		ConsoleWidget->AddSystemLog(InText, bInIsError);
	}
}
