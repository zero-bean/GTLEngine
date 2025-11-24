#include "pch.h"
#include "SymbolServerManager.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Static 멤버 변수
std::wstring FSymbolServerManager::s_LocalCachePath;
std::wstring FSymbolServerManager::s_SymbolServerPath;
bool FSymbolServerManager::s_bInitialized = false;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 네트워크 경로 핑 체크 (3번 시도)
bool FSymbolServerManager::PingNetworkPath(const std::wstring& NetworkPath, DWORD TimeoutMs)
{
	if (NetworkPath.empty()) return false;

	// 3번 시도
	for (int i = 0; i < 3; ++i)
	{
		// GetFileAttributesW를 비동기적으로 호출하기 위해 스레드 사용
		DWORD Attributes = INVALID_FILE_ATTRIBUTES;
		bool bCompleted = false;

		std::thread PingThread([&]() {
			Attributes = GetFileAttributesW(NetworkPath.c_str());
			bCompleted = true;
		});

		// 타임아웃 대기
		auto Start = std::chrono::steady_clock::now();
		while (!bCompleted)
		{
			auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - Start).count();

			if (Elapsed > TimeoutMs)
			{
				// 타임아웃 발생 - 스레드는 백그라운드에서 계속 실행되지만 기다리지 않음
				PingThread.detach();
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// 성공하면 즉시 반환
		if (bCompleted && Attributes != INVALID_FILE_ATTRIBUTES)
		{
			if (PingThread.joinable()) PingThread.join();
			return true;
		}

		// 실패한 경우 스레드가 아직 실행 중이면 detach
		if (PingThread.joinable()) PingThread.join();
	}

	return false;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 초기화
void FSymbolServerManager::Initialize(const std::wstring& LocalCachePath, const std::wstring& SymbolServerPath)
{
	s_LocalCachePath = LocalCachePath;

	// 로컬 캐시 디렉토리 생성
	std::filesystem::create_directories(s_LocalCachePath);

	// 심볼 서버가 지정된 경우 핑 체크
	if (!SymbolServerPath.empty())
	{
		wprintf(L"[SymbolServer] Checking server availability: %s\n", SymbolServerPath.c_str());

		if (PingNetworkPath(SymbolServerPath, 100))
		{
			s_SymbolServerPath = SymbolServerPath;
			wprintf(L"[SymbolServer] Initialized: Cache=%s, Server=%s\n",
				s_LocalCachePath.c_str(), s_SymbolServerPath.c_str());
		}
		else
		{
			s_SymbolServerPath = L"";
			wprintf(L"[SymbolServer] Server unavailable (timeout after 3 attempts), using local cache only\n");
		}
	}
	else
	{
		s_SymbolServerPath = L"";
		wprintf(L"[SymbolServer] Initialized with local cache only (no server specified)\n");
	}

	s_bInitialized = true;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 심볼 검색 경로 생성
std::wstring FSymbolServerManager::GetSymbolSearchPath()
{
	if (!s_bInitialized) return L"";

	// 서버가 없으면 로컬 캐시만 사용
	if (s_SymbolServerPath.empty())
	{
		return s_LocalCachePath;
	}

	// srv*[로컬캐시]*[심볼서버] 형식
	std::wstringstream Wss;
	Wss << L"srv*" << s_LocalCachePath << L"*" << s_SymbolServerPath;
	return Wss.str();
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// EXE/DLL에서 PDB 정보 추출
bool FSymbolServerManager::ExtractPdbInfo(const std::wstring& ExePath, GUID& OutGuid, DWORD& OutAge)
{
	std::ifstream File(ExePath, std::ios::binary);
	if (!File.is_open()) return false;

	// DOS 헤더 읽기
	IMAGE_DOS_HEADER DosHeader;
	File.read(reinterpret_cast<char*>(&DosHeader), sizeof(DosHeader));
	if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) return false;

	// NT 헤더로 이동
	File.seekg(DosHeader.e_lfanew);
	IMAGE_NT_HEADERS64 NtHeader;
	File.read(reinterpret_cast<char*>(&NtHeader), sizeof(NtHeader));
	if (NtHeader.Signature != IMAGE_NT_SIGNATURE) return false;

	// Debug Directory 찾기
	DWORD DebugDirRva = NtHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	DWORD DebugDirSize = NtHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	if (DebugDirRva == 0 || DebugDirSize == 0) return false;

	// Debug Directory를 파일 오프셋으로 변환
	DWORD FileOffset = 0;
	IMAGE_SECTION_HEADER SectionHeader;
	for (int i = 0; i < NtHeader.FileHeader.NumberOfSections; ++i)
	{
		File.read(reinterpret_cast<char*>(&SectionHeader), sizeof(SectionHeader));
		if (DebugDirRva >= SectionHeader.VirtualAddress &&
			DebugDirRva < SectionHeader.VirtualAddress + SectionHeader.SizeOfRawData)
		{
			FileOffset = SectionHeader.PointerToRawData + (DebugDirRva - SectionHeader.VirtualAddress);
			break;
		}
	}
	if (FileOffset == 0) return false;

	// Debug Directory 읽기
	File.seekg(FileOffset);
	IMAGE_DEBUG_DIRECTORY DebugDir;
	File.read(reinterpret_cast<char*>(&DebugDir), sizeof(DebugDir));

	// CodeView (RSDS) 정보 읽기
	if (DebugDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) return false;
	File.seekg(DebugDir.PointerToRawData);

	struct RSDS_HEADER
	{
		DWORD Signature;  // 'RSDS'
		GUID Guid;
		DWORD Age;
		char PdbPath[1];
	};

	RSDS_HEADER RsdsHeader;
	File.read(reinterpret_cast<char*>(&RsdsHeader), sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD));

	if (RsdsHeader.Signature != 0x53445352) return false;  // 'RSDS'

	OutGuid = RsdsHeader.Guid;
	OutAge = RsdsHeader.Age;
	return true;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 심볼 서버에 PDB 존재 확인
bool FSymbolServerManager::IsSymbolOnServer(const std::wstring& PdbFileName, const GUID& PdbGuid, DWORD PdbAge)
{
	if (!s_bInitialized || s_SymbolServerPath.empty()) return false;

	// GUID를 대문자 16진수 문자열로 변환 (하이픈 제거)
	wchar_t GuidStr[33];
	swprintf_s(GuidStr, L"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
		PdbGuid.Data1, PdbGuid.Data2, PdbGuid.Data3,
		PdbGuid.Data4[0], PdbGuid.Data4[1], PdbGuid.Data4[2], PdbGuid.Data4[3],
		PdbGuid.Data4[4], PdbGuid.Data4[5], PdbGuid.Data4[6], PdbGuid.Data4[7]);

	// 심볼 서버 경로 구성: [서버]\[PdbName]\[GUID+Age]\[PdbName]
	std::wstringstream Wss;
	Wss << s_SymbolServerPath << L"\\" << PdbFileName << L"\\" << GuidStr << std::hex << std::uppercase << PdbAge << L"\\" << PdbFileName;

	return std::filesystem::exists(Wss.str());
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// symstore.exe 찾기
std::wstring FSymbolServerManager::FindSymstoreExe()
{
	// Windows SDK 경로들
	const wchar_t* SdkPaths[] = {
		L"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\symstore.exe",
		L"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x86\\symstore.exe",
		L"C:\\Program Files\\Debugging Tools for Windows (x64)\\symstore.exe",
	};

	for (const wchar_t* Path : SdkPaths)
	{
		if (std::filesystem::exists(Path)) return Path;
	}

	return L"";
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 심볼 서버에 업로드
bool FSymbolServerManager::UploadSymbolToServer(
	const std::wstring& ExePath,
	const std::wstring& PdbPath,
	const std::wstring& ProductName,
	const std::wstring& Version)
{
	if (!s_bInitialized || s_SymbolServerPath.empty())
	{
		wprintf(L"[SymbolServer] Upload skipped (server unavailable)\n");
		return false;
	}

	// PDB 파일 존재 확인
	if (!std::filesystem::exists(PdbPath))
	{
		OutputDebugStringW(L"[SymbolServer] PDB file not found\n");
		wprintf(L"[SymbolServer] PDB file not found: %s\n", PdbPath.c_str());
		return false;
	}
	wprintf(L"[SymbolServer] PDB file found: %s\n", PdbPath.c_str());

	// symstore.exe 찾기
	std::wstring SymstorePath = FindSymstoreExe();
	if (SymstorePath.empty())
	{
		OutputDebugStringW(L"[SymbolServer] symstore.exe not found\n");
		wprintf(L"[SymbolServer] symstore.exe not found (install Windows SDK)\n");
		return false;
	}
	wprintf(L"[SymbolServer] Using symstore: %s\n", SymstorePath.c_str());

	// symstore.exe 명령 구성
	std::wstringstream Cmd;
	Cmd << L"\"" << SymstorePath << L"\" add"
		<< L" /f \"" << PdbPath << L"\""
		<< L" /s \"" << s_SymbolServerPath << L"\""
		<< L" /t \"" << ProductName << L"\""
		<< L" /v \"" << Version << L"\""
		<< L" /compress";

	// 프로세스 실행
	STARTUPINFOW Si = { sizeof(Si) };
	PROCESS_INFORMATION Pi = {};

	std::wstring CmdLine = Cmd.str();
	if (!CreateProcessW(NULL, &CmdLine[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &Si, &Pi))
	{
		OutputDebugStringW(L"[SymbolServer] Failed to run symstore.exe\n");
		return false;
	}

	// 완료 대기 (최대 30초)
	WaitForSingleObject(Pi.hProcess, 30000);

	DWORD ExitCode = 0;
	GetExitCodeProcess(Pi.hProcess, &ExitCode);

	CloseHandle(Pi.hProcess);
	CloseHandle(Pi.hThread);

	if (ExitCode == 0)
	{
		OutputDebugStringW(L"[SymbolServer] Upload successful\n");
		wprintf(L"[SymbolServer] Upload successful!\n");
		return true;
	}
	else
	{
		std::wstringstream Msg;
		Msg << L"[SymbolServer] Upload failed with code " << ExitCode << L"\n";
		OutputDebugStringW(Msg.str().c_str());
		wprintf(L"[SymbolServer] Upload failed with code %d\n", ExitCode);
		return false;
	}
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 필요 시 자동 업로드
bool FSymbolServerManager::UploadSymbolsIfNeeded(const std::wstring& ExePath, const std::wstring& PdbPath)
{
	if (!s_bInitialized) return false;

	wprintf(L"[SymbolServer] Checking if upload needed...\n");

	// 1. EXE에서 PDB 정보 추출
	GUID PdbGuid;
	DWORD PdbAge;
	if (!ExtractPdbInfo(ExePath, PdbGuid, PdbAge))
	{
		OutputDebugStringW(L"[SymbolServer] Failed to extract PDB info from EXE\n");
		wprintf(L"[SymbolServer] Failed to extract PDB info from EXE\n");
		return false;
	}

	// 2. PDB 파일명 추출
	std::wstring PdbFileName = std::filesystem::path(PdbPath).filename();

	// 3. 심볼 서버에 이미 있는지 확인
	if (IsSymbolOnServer(PdbFileName, PdbGuid, PdbAge))
	{
		OutputDebugStringW(L"[SymbolServer] Symbol already exists on server\n");
		wprintf(L"[SymbolServer] Symbol already exists on server, skipping upload\n");
		return true;
	}

	// 4. 없으면 업로드
	OutputDebugStringW(L"[SymbolServer] Symbol not found on server, uploading...\n");
	wprintf(L"[SymbolServer] Symbol not found on server, uploading...\n");
	return UploadSymbolToServer(ExePath, PdbPath);
}
