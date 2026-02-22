#include "pch.h"
#include "Widgets/ConsoleWidget.h"

IMPLEMENT_CLASS(UGlobalConsole)

UConsoleWidget* UGlobalConsole::ConsoleWidget = nullptr;

void UGlobalConsole::Initialize()
{
    // Nothing special to initialize
}

void UGlobalConsole::Shutdown()
{
    ConsoleWidget = nullptr;
}

void UGlobalConsole::SetConsoleWidget(UConsoleWidget* InConsoleWidget)
{
    ConsoleWidget = InConsoleWidget;
    if (InConsoleWidget)
    {
        UE_LOG("GlobalConsole: ConsoleWidget set successfully\n");
    }
    else
    {
        UE_LOG("GlobalConsole: ConsoleWidget set to null\n");
    }
}

UConsoleWidget* UGlobalConsole::GetConsoleWidget()
{
    return ConsoleWidget;
}

void UGlobalConsole::Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogV(fmt, args);
    va_end(args);
}

void UGlobalConsole::LogV(const char* fmt, va_list args)
{
    // 항상 OutputDebugString 출력 (Visual Studio Output 창에서 확인 가능)
    char tmp[1024];
    vsnprintf_s(tmp, _countof(tmp), fmt, args);
    OutputDebugStringA(tmp);
    OutputDebugStringA("\n");

#ifdef _EDITOR
    // 에디터에서는 ConsoleWidget에도 출력
    if (ConsoleWidget)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        ConsoleWidget->VAddLog(fmt, args_copy);
        va_end(args_copy);
    }
#endif
}

// Global C functions for compatibility
extern "C" void ConsoleLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    UGlobalConsole::LogV(fmt, args);
    va_end(args);
}

extern "C" void ConsoleLogV(const char* fmt, va_list args)
{
    UGlobalConsole::LogV(fmt, args);
}
