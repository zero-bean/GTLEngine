#include "pch.h"
#include "CrashHandler.h"
#include "SymbolServerManager.h"
#include <filesystem>
#include <chrono>
#include <sstream>
#include <atomic>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Static 전역 변수
namespace
{
	std::atomic_bool g_bInitialized{ false };      // 초기화 여부
	std::atomic_bool g_bWritingDump{ false };      // 덤프 작성 중 플래그 (중복 방지)
	std::atomic_int g_NextDumpProfile{ 0 };        // 다음 덤프 프로파일 (0=Full, 1=DataSegsOnly)
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 초기화
void FCrashHandler::Initialize()
{
	// 중복 초기화 방지
	bool bExpected = false;
	if (!g_bInitialized.compare_exchange_strong(bExpected, true)) return;

	// 프로젝트 루트 경로 획득 및 덤프 폴더 생성 (Saved/Crashes/)
	wchar_t ExePath[MAX_PATH];
	GetModuleFileNameW(nullptr, ExePath, MAX_PATH);
	std::filesystem::path ProjectRoot = std::filesystem::path(ExePath).parent_path().parent_path().parent_path();
	std::filesystem::create_directories(ProjectRoot / L"Saved" / L"Crashes");

	// 심볼 서버 초기화 (로컬 캐시 + 네트워크 심볼 서버)
	// 우선순위: 1) 로컬 서버 2) 네트워크 서버 3) 로컬 캐시만
	std::wstring SymbolServerPath;

	// 1. 로컬에 심볼 서버가 있는지 확인
	DWORD Attrs = GetFileAttributesW(L"C:\\SymbolServer");
	if (Attrs != INVALID_FILE_ATTRIBUTES && (Attrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		SymbolServerPath = L"C:\\SymbolServer";
	}
	else
	{
		// 2. 네트워크 심볼 서버 접근 가능한지 확인 (타임아웃 방지)
		Attrs = GetFileAttributesW(L"\\\\172.21.11.95\\SymbolServer");
		if (Attrs != INVALID_FILE_ATTRIBUTES && (Attrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			SymbolServerPath = L"\\\\172.21.11.95\\SymbolServer";
		}
		else
		{
			// 3. 서버 접근 불가 - 로컬 캐시만 사용
			OutputDebugStringW(L"[SymbolServer] Server unavailable, using local cache only\n");
			SymbolServerPath = L"";  // 빈 경로 = 캐시만 사용
		}
	}

	if (!SymbolServerPath.empty())
	{
		FSymbolServerManager::Initialize(L"C:\\SymbolCache", SymbolServerPath);
	}

	// 시스템 에러 메시지 박스 비활성화
	SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);

	// Unhandled Exception Filter 등록
	SetUnhandledExceptionFilter(OnUnhandledException);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 예외 핸들러
LONG WINAPI FCrashHandler::OnUnhandledException(EXCEPTION_POINTERS* ExceptionInfo)
{
	// Step 1: 예외 타입 식별
	const char* ExceptionDesc = "Unknown";
	if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
	{
		switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:       ExceptionDesc = "Access Violation"; break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  ExceptionDesc = "Array Bounds Exceeded"; break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:     ExceptionDesc = "Integer Divide by Zero"; break;
		case EXCEPTION_STACK_OVERFLOW:         ExceptionDesc = "Stack Overflow"; break;
		}
	}

	// Step 2: 스택 트레이스 수집 (심볼 서버 경로 설정)
	HANDLE Process = GetCurrentProcess();
	std::wstring SymbolPath = FSymbolServerManager::GetSymbolSearchPath();
	SymInitializeW(Process, SymbolPath.empty() ? NULL : SymbolPath.c_str(), TRUE);
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

	// Context 복사 (원본 보호)
	CONTEXT ContextCopy = ExceptionInfo ? *ExceptionInfo->ContextRecord : CONTEXT{};

	// 스택 프레임 초기화 (플랫폼별 레지스터 설정)
	STACKFRAME64 Stack = {};
#ifdef _M_X64
	DWORD MachineType = IMAGE_FILE_MACHINE_AMD64;
	Stack.AddrPC.Offset = ContextCopy.Rip;
	Stack.AddrFrame.Offset = ContextCopy.Rbp;
	Stack.AddrStack.Offset = ContextCopy.Rsp;
#else
	DWORD MachineType = IMAGE_FILE_MACHINE_I386;
	Stack.AddrPC.Offset = ContextCopy.Eip;
	Stack.AddrFrame.Offset = ContextCopy.Ebp;
	Stack.AddrStack.Offset = ContextCopy.Esp;
#endif
	Stack.AddrPC.Mode = Stack.AddrFrame.Mode = Stack.AddrStack.Mode = AddrModeFlat;

	// 스택 워크 (최대 64프레임, 처음 3개만 메시지 박스에 표시)
	char StackFrames[3][256] = {};
	int32 CapturedCount = 0;

	for (int32 FrameNum = 0; FrameNum < 64 && StackWalk64(MachineType, Process, GetCurrentThread(),
		&Stack, &ContextCopy, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL); ++FrameNum)
	{
		char Buf[256];

		// 심볼 정보 조회
		char SymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO Symbol = (PSYMBOL_INFO)SymbolBuffer;
		Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		Symbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(Process, Stack.AddrPC.Offset, nullptr, Symbol))
		{
			// 소스 파일 정보 조회
			IMAGEHLP_LINE64 Line = { sizeof(IMAGEHLP_LINE64) };
			DWORD LineDisp;
			if (SymGetLineFromAddr64(Process, Stack.AddrPC.Offset, &LineDisp, &Line))
			{
				// 파일명만 표시 (경로 제거)
				const char* FileName = strrchr(Line.FileName, '\\');
				sprintf_s(Buf, "[%02d] %s (%s:%d)\n", FrameNum, Symbol->Name, FileName ? FileName + 1 : Line.FileName, Line.LineNumber);
			}
			else
			{
				sprintf_s(Buf, "[%02d] %s\n", FrameNum, Symbol->Name);
			}
		}
		else
		{
			// 심볼 정보 없음 - 주소만 표시
			sprintf_s(Buf, "[%02d] 0x%p\n", FrameNum, (void*)Stack.AddrPC.Offset);
		}

		// 처음 3개 프레임만 메시지 박스용으로 저장
		if (CapturedCount < 3) strcpy_s(StackFrames[CapturedCount++], Buf);
	}

	SymCleanup(Process);

	// Step 3: 메시지 박스 생성
	char MsgBox[1024];
	sprintf_s(MsgBox, "Exception: %s\n\n%s", ExceptionDesc, 
		CapturedCount > 0 ? StackFrames[0] : "[??] Stack unavailable");
	for (int32 i = 1; i < CapturedCount; ++i) strcat_s(MsgBox, StackFrames[i]);
	MessageBoxA(NULL, MsgBox, "Crash Detected", MB_OK | MB_ICONERROR | MB_TOPMOST);

	// Step 4: 미니덤프 작성
	WriteMiniDump(ExceptionInfo);

	return EXCEPTION_EXECUTE_HANDLER;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 미니덤프 작성
void FCrashHandler::WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo)
{
	// 중복 덤프 작성 방지
	bool bExpected = false;
	if (!g_bWritingDump.compare_exchange_strong(bExpected, true)) return;

	// 프로젝트 루트 경로 획득
	wchar_t ExePath[MAX_PATH];
	GetModuleFileNameW(nullptr, ExePath, MAX_PATH);
	std::filesystem::path ProjectRoot = std::filesystem::path(ExePath).parent_path().parent_path().parent_path();

	// 타임스탬프 생성
	auto Now = std::chrono::system_clock::now();
	std::time_t Tt = std::chrono::system_clock::to_time_t(Now);
	tm Tms;
	localtime_s(&Tms, &Tt);

	// 덤프 파일명 생성 (Crash_[PID]_[YYYYMMDD_HHMMSS].dmp)
	std::wstringstream Wss;
	Wss << L"Crash_" << GetCurrentProcessId() << L"_" << std::put_time(&Tms, L"%Y%m%d_%H%M%S") << L".dmp";

	// 덤프 파일 생성 (Saved/Crashes/)
	std::filesystem::path DumpPath = ProjectRoot / L"Saved" / L"Crashes" / Wss.str();
	HANDLE HFile = CreateFileW(DumpPath.c_str(), 
		GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (HFile == INVALID_HANDLE_VALUE)
	{
		g_bWritingDump.store(false);
		return;
	}

	// 덤프 타입
	MINIDUMP_EXCEPTION_INFORMATION DumpInfo = { GetCurrentThreadId(), ExceptionInfo, FALSE };
	MINIDUMP_TYPE DumpType = static_cast<MINIDUMP_TYPE>(
		MiniDumpNormal |
		MiniDumpWithDataSegs |
		MiniDumpWithHandleData |
		MiniDumpWithThreadInfo |
		MiniDumpWithIndirectlyReferencedMemory
	);

	// 덤프 작성
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
		HFile, DumpType, ExceptionInfo ? &DumpInfo : nullptr, NULL, NULL);

	CloseHandle(HFile);
	g_bWritingDump.store(false);
}

void FCrashHandler::SetNextDumpProfileDataSegsOnly()
{
	g_NextDumpProfile.store(1);
}
