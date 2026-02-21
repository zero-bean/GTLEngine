#include "pch.h"
#include "CrashHandler.h"
#include "ObjectFactory.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <random>

namespace
{
    std::atomic_bool g_Initialized{ false };
    std::atomic_bool g_WritingDump{ false };
    // 0 = default (full), 1 = data-segs only
    std::atomic_int g_NextDumpProfile{ 0 };
    // continuous random deletion mode
    std::atomic_bool g_Bombard{ false };
    std::atomic_int g_BombardPerFrame{ 1 };
}

void FCrashHandler::Initialize()
{
    // 전역 예외 핸들러 등록 (1회만)
    bool expected = false;
    if (!g_Initialized.compare_exchange_strong(expected, true))
        return;

    // Reduce OS UI that could block dump creation under debugger-less runs
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
    SetUnhandledExceptionFilter(FCrashHandler::OnUnhandledException);
}

LONG WINAPI FCrashHandler::OnUnhandledException(EXCEPTION_POINTERS* ExceptionInfo)
{
    WriteMiniDump(ExceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER; // 프로세스 종료
}

void FCrashHandler::WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo)
{
    // 재진입 방지
    bool expected = false;
    if (!g_WritingDump.compare_exchange_strong(expected, true))
        return;

    // exe가 있는 폴더에 덤프 생성 (간단하고 항상 성공)
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    tm tms{};
#if defined(_MSC_VER)
    localtime_s(&tms, &tt);
#else
    tms = *std::localtime(&tt);
#endif

    std::wstringstream wss;
    wss << L"Crash_" << GetCurrentProcessId() << L"_"
        << std::put_time(&tms, L"%Y%m%d_%H%M%S") << L".dmp";

    // 덤프를 exe와 같은 폴더에 생성
    std::filesystem::path dumpPath = exeDir / wss.str();

    HANDLE hFile = CreateFileW(
        dumpPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD le = GetLastError();
        wchar_t buf[256];
        swprintf_s(buf, L"[CrashHandler] CreateFileW failed at %s (LE=%lu)\n", dumpPath.c_str(), le);
        OutputDebugStringW(buf);
        g_WritingDump.store(false);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo{};
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = ExceptionInfo;
    dumpInfo.ClientPointers = TRUE;

    // Select dump type (one-shot profile)
    MINIDUMP_TYPE dumpType = MiniDumpWithFullMemory;
    int profile = g_NextDumpProfile.exchange(0);
    if (profile == 1)
    {
        dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithDataSegs);
    }

    BOOL ok = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        dumpType,
        &dumpInfo,
        NULL,
        NULL);

    CloseHandle(hFile);

    if (ok)
    {
        std::wstringstream msg;
        msg << L"[CrashHandler] MiniDump written: " << dumpPath.c_str() << L"\n";
        OutputDebugStringW(msg.str().c_str());
    }
    else
    {
        OutputDebugStringW(L"[CrashHandler] MiniDumpWriteDump failed\n");
    }

    g_WritingDump.store(false);
}

void FCrashHandler::SetNextDumpProfileDataSegsOnly()
{
    g_NextDumpProfile.store(1);
}

void FCrashHandler::RandomCrash()
{
    // Select a random valid UObject from GUObjectArray
    int32 total = GUObjectArray.Num();
    if (total <= 0)
    {
        OutputDebugStringA("[CrashHandler] RandomCrash: no objects to delete.\n");
        SetNextDumpProfileDataSegsOnly();
        return;
    }

    TArray<int32> valid;
    valid.Reserve(total);
    for (int32 i = 0; i < total; ++i)
    {
        if (GUObjectArray[i] != nullptr)
            valid.Add(i);
    }
    if (valid.Num() == 0)
    {
        OutputDebugStringA("[CrashHandler] RandomCrash: all slots null.\n");
        SetNextDumpProfileDataSegsOnly();
        return;
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int32> dist(0, valid.Num() - 1);
    int32 pick = valid[dist(rng)];

    UObject* victim = GUObjectArray[pick];
    std::string victimName = victim ? victim->GetName() : std::string("<null>");
    std::string victimClass = (victim && victim->GetClass() && victim->GetClass()->Name)
        ? victim->GetClass()->Name : std::string("<null>");

    // Perform deletion; do NOT force a crash here — we want the real fault site.
    ObjectFactory::DeleteObject(victim);

    std::stringstream ss;
    ss << "[CrashHandler] RandomCrash deleted index=" << pick
       << " Name='" << victimName << "' Class='" << victimClass << "'\n";
    OutputDebugStringA(ss.str().c_str());

    // Make the next crash compact (stack + exception + data segs)
    SetNextDumpProfileDataSegsOnly();
}

void FCrashHandler::EnableRandomCrashBombard(bool bEnable, int32 DeletionsPerFrame)
{
    g_Bombard.store(bEnable);
    if (DeletionsPerFrame <= 0) DeletionsPerFrame = 1;
    g_BombardPerFrame.store(DeletionsPerFrame);
    if (bEnable)
    {
        OutputDebugStringA("[CrashHandler] RandomCrash bombard enabled\n");
    }
    else
    {
        OutputDebugStringA("[CrashHandler] RandomCrash bombard disabled\n");
    }
}

void FCrashHandler::RandomCrashTick()
{
    if (!g_Bombard.load()) return;
    int n = g_BombardPerFrame.load();
    for (int i = 0; i < n; ++i)
    {
        // Delete one candidate; if a real crash happens, we won't get here next frame
        RandomCrash();
    }
}
