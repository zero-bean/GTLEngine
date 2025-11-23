#include "pch.h"
#include "EditorEngine.h"
#include "Source/Runtime/Debug/CrashHandler.h"
#include "Source/Runtime/Debug/CrashCommand.h"
#include "Source/Runtime/Debug/Console.h" 
// C runtime for freopen_s
#include <cstdio>


#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

extern void StartConsoleThread();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif

    FCrashHandler::Initialize();  

    // Attach to parent console (if launched from cmd) or create a new console.
    //if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    FConsole& Console = FConsole::GetInstance();

    // 크래시 테스트용 명령어 등록
    // 사용법: 콘솔에서 "Crash" 입력
    // 효과: 매 프레임마다 메모리를 점진적으로 오염시켜 자연스럽게 크래시 발생
    Console.RegisterCommand(L"Crash", []() {
        CCrashCommand::EnableCorruptionMode();
    }); 

    StartConsoleThread();

    if (!GEngine.Startup(hInstance))
        return -1;

    try
    {
        GEngine.MainLoop();
        GEngine.Shutdown();
    }
    catch (const std::exception& e)
    {
        throw;
    }

    return 0;
}
