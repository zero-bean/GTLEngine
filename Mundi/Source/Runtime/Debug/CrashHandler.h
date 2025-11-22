#pragma once
#include <Windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "dbghelp.lib")

/**
 * @brief 크래시 발생 시 덤프 파일(.dmp) 생성 및 디버그 정보 출력
 * - Unhandled Exception 발생 시 자동으로 미니덤프 파일 생성
 * - 스택 트레이스를 콘솔 및 메시지 박스에 출력
 * - 덤프 파일은 Saved/Crashes/ 폴더에 저장
 */
class FCrashHandler
{
public:
	/**
	 * 크래시 핸들러 초기화 (프로그램 시작 시 1회 호출)
	 * - Unhandled Exception Filter 등록
	 * - 덤프 폴더 생성
	 */
	static void Initialize();

	/**
	 * Unhandled Exception 발생 시 호출되는 핸들러
	 * - 스택 트레이스 수집 및 출력
	 * - 메시지 박스 표시
	 * - 미니덤프 파일 작성
	 */
	static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* ExceptionInfo);

	/**
	 * 미니덤프 파일 작성 (Saved/Crashes/*.dmp)
	 */
	static void WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo);

	/**
	 * 다음 덤프를 경량 프로파일로 설정
	 * - MiniDumpNormal + MiniDumpWithDataSegs (전체 메모리 대신 데이터 세그먼트만)
	 */
	static void SetNextDumpProfileDataSegsOnly();
};
