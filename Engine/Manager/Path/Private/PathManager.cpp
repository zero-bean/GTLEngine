#include "pch.h"
#include "Manager/Path/Public/PathManager.h"

IMPLEMENT_SINGLETON(UPathManager)

UPathManager::UPathManager()
{
	Init();
}

UPathManager::~UPathManager() = default;

void UPathManager::Init()
{
	InitializeRootPath();
	GetEssentialPath();
	ValidateAndCreateDirectories();

	cout << "[PathManager] Initialized Successfully" << endl;
	cout << "[PathManager] Solution Path: " << RootPath << endl;
	cout << "[PathManager] Asset Path: " << AssetPath << endl;
}

/**
 * @brief 프로그램 실행 파일을 기반으로 Root Path 세팅하는 함수
 */
void UPathManager::InitializeRootPath()
{
	// Get Execution File Path
	wchar_t ProgramPath[MAX_PATH];
	GetModuleFileNameW(nullptr, ProgramPath, MAX_PATH);

	// Add Root Path
	RootPath = path(ProgramPath).parent_path();
}

/**
 * @brief 프로그램 실행에 필요한 필수 경로를 추가하는 함수
 */
void UPathManager::GetEssentialPath()
{
	// Add Essential

	AssetPath = RootPath / L"Asset";
	ShaderPath = AssetPath / L"Shader";
	TexturePath = AssetPath / "Texture";
	ModelPath = AssetPath / "Model";
	AudioPath = AssetPath / "Audio";
	WorldPath = AssetPath / "World";
	ConfigPath = AssetPath / "Config";
}

/**
 * @brief 경로 유효성을 확인하고 없는 디렉토리를 생성하는 함수
 */
void UPathManager::ValidateAndCreateDirectories() const
{
	TArray<path> DirectoriesToCreate = {
		AssetPath,
		ShaderPath,
		TexturePath,
		ModelPath,
		AudioPath,
		WorldPath,
		ConfigPath
	};

	for (const auto& Directory : DirectoriesToCreate)
	{
		try
		{
			if (!exists(Directory))
			{
				create_directories(Directory);
				cout << "[PathManager] Created Directory: " << Directory << endl;
			}
			else
			{
				cout << "[PathManager] Directory Exists: " << Directory << endl;
			}
		}
		catch (const filesystem::filesystem_error& e)
		{
			cout << "[PathManager] Failed To Create Directory " <<
				Directory << ": " << e.what() << endl;
			assert(!"Asset 경로 생성 에러 발생");
		}
	}
}
