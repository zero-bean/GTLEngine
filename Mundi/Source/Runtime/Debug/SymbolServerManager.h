#pragma once
#include <Windows.h>
#include <DbgHelp.h>
#include <string>
#include <filesystem>

#pragma comment(lib, "dbghelp.lib")

/**
 * @brief 심볼 서버 통합 관리자
 *
 * 기능:
 * 1. 심볼 검색: 로컬 캐시 → 심볼 서버 → 로컬 빌드 디렉토리
 * 2. 자동 업로드: 로컬에서 생성한 PDB를 심볼 서버에 자동 업로드
 * 3. 팀 공유: 모든 팀원이 동일한 심볼 서버 사용
 *
 * 사용 예시:
 * ```cpp
 * FSymbolServerManager::Initialize(L"C:\\SymbolCache", L"\\\\PC-NAME\\SymbolServer");
 * FSymbolServerManager::UploadSymbolsIfNeeded(L"Binaries\\Debug\\Mundi.exe", L"Binaries\\Debug\\Mundi.pdb");
 * ```
 */
class FSymbolServerManager
{
public:
	/**
	 * @brief 심볼 서버 관리자 초기화
	 * @param LocalCachePath 로컬 심볼 캐시 경로 (예: C:\SymbolCache)
	 * @param SymbolServerPath 네트워크 심볼 서버 경로 (예: \\PC-NAME\SymbolServer)
	 */
	static void Initialize(const std::wstring& LocalCachePath, const std::wstring& SymbolServerPath);

	/**
	 * @brief DbgHelp에 심볼 검색 경로 설정
	 *
	 * 검색 순서:
	 * 1. 로컬 캐시 (C:\SymbolCache)
	 * 2. 심볼 서버 (srv*C:\SymbolCache*\\PC-NAME\SymbolServer)
	 *
	 * @return 설정된 심볼 경로 문자열
	 */
	static std::wstring GetSymbolSearchPath();

	/**
	 * @brief 심볼 서버에 PDB가 있는지 확인
	 * @param PdbFileName PDB 파일명 (예: Mundi.pdb)
	 * @param PdbGuid PDB GUID (exe/dll에서 추출)
	 * @param PdbAge PDB Age (exe/dll에서 추출)
	 * @return 심볼 서버에 해당 PDB가 존재하면 true
	 */
	static bool IsSymbolOnServer(const std::wstring& PdbFileName, const GUID& PdbGuid, DWORD PdbAge);

	/**
	 * @brief EXE/DLL에서 PDB 정보 추출 (GUID, Age)
	 * @param ExePath EXE/DLL 파일 경로
	 * @param OutGuid PDB GUID 출력
	 * @param OutAge PDB Age 출력
	 * @return 성공 시 true
	 */
	static bool ExtractPdbInfo(const std::wstring& ExePath, GUID& OutGuid, DWORD& OutAge);

	/**
	 * @brief PDB를 심볼 서버에 업로드 (symstore.exe 사용)
	 *
	 * @param ExePath EXE/DLL 파일 경로 (PDB GUID 추출용)
	 * @param PdbPath PDB 파일 경로
	 * @param ProductName 제품 이름 (예: Mundi)
	 * @param Version 버전 정보 (예: 1.0.0)
	 * @return 업로드 성공 시 true
	 */
	static bool UploadSymbolToServer(
		const std::wstring& ExePath,
		const std::wstring& PdbPath,
		const std::wstring& ProductName = L"Mundi",
		const std::wstring& Version = L"1.0.0"
	);

	/**
	 * @brief 필요 시 자동으로 심볼 업로드
	 *
	 * 동작:
	 * 1. 심볼 서버에 이미 있으면 스킵
	 * 2. 없으면 자동으로 업로드
	 *
	 * @param ExePath EXE/DLL 파일 경로
	 * @param PdbPath PDB 파일 경로
	 * @return 업로드 수행 또는 이미 존재하면 true
	 */
	static bool UploadSymbolsIfNeeded(const std::wstring& ExePath, const std::wstring& PdbPath);

	/**
	 * @brief symstore.exe 경로 찾기
	 * @return symstore.exe 전체 경로 (없으면 빈 문자열)
	 */
	static std::wstring FindSymstoreExe();

private:
	static std::wstring s_LocalCachePath;    // 로컬 심볼 캐시
	static std::wstring s_SymbolServerPath;  // 네트워크 심볼 서버
	static bool s_bInitialized;
};
