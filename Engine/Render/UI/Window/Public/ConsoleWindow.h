#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

using std::streambuf;

class UConsoleWindow;

struct FLogEntry {
	ELogType Type;
	FString Message;
};

/**
 * @brief Custom Stream Buffer
 * Redirects Output to ConsoleWindow
 */
class ConsoleStreamBuffer : public streambuf
{
public:
	ConsoleStreamBuffer(UConsoleWindow* InConsole, bool bInIsError = false)
		: Console(InConsole), bIsError(bInIsError)
	{
	}

protected:
	// Stream Buffer 함수 Override
	int overflow(int InCharacter) override;
	streamsize xsputn(const char* InString, streamsize InCount) override;

private:
	UConsoleWindow* Console;
	FString Buffer;
	bool bIsError;
};

/**
 * @brief Simple Console Window
 * Shows Logs & Processes Commands
 */
class UConsoleWindow : public UUIWindow
{
public:
	static UConsoleWindow& GetInstance();

	void Initialize() override;
	void Render() override;
	void Update() override;
	void Cleanup() override;

	void AddLog(const char* fmt, ...);
	void AddLog(ELogType InType, const char* fmt, ...);
	void ClearLog();

	void ProcessCommand(const char* InCommand);
	void ExecuteTerminalCommand(const char* InCommand);

	void InitializeSystemRedirect();
	void CleanupSystemRedirect();
	void AddSystemLog(const char* InText, bool bInIsError = false);
	void AddSystemLog(ELogType InType, const char* InText);

private:
	// Helper Functions
	static ImVec4 GetColorByLogType(ELogType InType);

public:
	// Special Member Function
	explicit UConsoleWindow(const FUIWindowConfig& InConfig = FUIWindowConfig());
	~UConsoleWindow() override;
	UConsoleWindow(const UConsoleWindow&) = delete;
	UConsoleWindow& operator=(const UConsoleWindow&) = delete;
	UConsoleWindow(UConsoleWindow&&) = delete;
	UConsoleWindow& operator=(UConsoleWindow&&) = delete;

	bool IsSingleton() override { return true; }
private:
	// Command Input
	char InputBuf[256];
	TArray<FString> CommandHistory;
	int HistoryPosition;

	// Log Output
	TArray<FLogEntry> LogItems;
	bool bIsAutoScroll;
	bool bIsScrollToBottom;

	// Stream Redirection
	ConsoleStreamBuffer* ConsoleOutputBuffer;
	ConsoleStreamBuffer* ConsoleErrorBuffer;
	streambuf* OriginalConsoleOutput;
	streambuf* OriginalConsoleError;
};
