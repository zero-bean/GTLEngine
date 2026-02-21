#include "pch.h"
#include "EditorEngine.h"
#include "CrashHandler.h"
#include <exception>

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif
    // Initialize global crash handler once per process
    FCrashHandler::Initialize();

    try
    {
        if (!GEngine.Startup(hInstance))
            return -1;

        GEngine.MainLoop();
        GEngine.Shutdown();
    }
    catch (const std::exception& e)
    {
        //OutputDebugStringA("[CrashHandler] std::exception caught in WinMain. Raising SEH to generate dump.\n");
        //OutputDebugStringA(e.what());
        //OutputDebugStringA("\n");
        //// Convert to SEH so UnhandledExceptionFilter can generate a minidump
        //RaiseException(0xE0000001, 0, 0, nullptr);
    }
    catch (...)
    {
        //OutputDebugStringA("[CrashHandler] unknown exception caught in WinMain. Raising SEH to generate dump.\n");
        //RaiseException(0xE0000002, 0, 0, nullptr);
    }

    return 0;
}

