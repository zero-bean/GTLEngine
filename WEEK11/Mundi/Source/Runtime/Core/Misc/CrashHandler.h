#pragma once

#include <Windows.h>
#include <DbgHelp.h>
#include <string>

#pragma comment(lib, "dbghelp.lib")

// FCrashHandler
// - 프로세스 전역 미처리 예외를 가로채 MiniDump 파일을 생성하는 유틸리티.
// - 전역(static) 함수들만 제공하므로 별도 인스턴스가 필요 없습니다.
// - 추후 덤프 경로/옵션 설정이 필요하면 싱글톤 도입 또는 정적 세터 추가를 고려하세요.
class FCrashHandler
{
public:
    // Initialize
    // - 프로세스 전역 Unhandled Exception Filter를 등록합니다.
    // - 여러 번 호출되어도 1회만 등록되도록 설계합니다.
    static void Initialize();

    // OnUnhandledException
    // - OS가 감지한 미처리 예외에 대해 호출되는 최상위 SEH 콜백입니다.
    // - MiniDump를 생성하고 EXCEPTION_EXECUTE_HANDLER를 반환해 종료를 진행합니다.
    static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* ExceptionInfo);

    // WriteMiniDump
    // - 전달된 EXCEPTION_POINTERS 컨텍스트로 MiniDump(.dmp)를 생성합니다.
    // - 기본적으로 실행 파일 경로 옆에 타임스탬프/프로세스ID를 포함한 파일명을 사용합니다.
    static void WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo);

    // Configure the next crash to capture a compact dump
    // (basic stack + exception info + data segments only)
    static void SetNextDumpProfileDataSegsOnly();

    // Randomly delete one managed UObject via ObjectFactory to provoke
    // a potential natural crash later (UAF or misuse). Does NOT force a crash.
    // Sets the next-dump profile to DataSegs-only so when it does crash,
    // the dump stays compact and focused.
    static void RandomCrash();

    // Continuous random deletion: when enabled, deletes a few objects every
    // frame until a crash naturally occurs. Use EnableRandomCrashBombard(true)
    // via console, and UWorld::Tick will call RandomCrashTick().
    static void EnableRandomCrashBombard(bool bEnable, int32 DeletionsPerFrame = 1);
    static void RandomCrashTick();

    // Note: we do not force a crash at deletion site; we want natural crash sites.
};

