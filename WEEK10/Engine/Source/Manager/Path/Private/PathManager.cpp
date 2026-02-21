#include "pch.h"
#include "Manager/Path/Public/PathManager.h"

IMPLEMENT_SINGLETON_CLASS(UPathManager, UObject)

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

	UE_LOG("PathManager: Initialized Successfully");
	UE_LOG("PathManager: Solution Path: %ls", RootPath.c_str());
	UE_LOG("PathManager: Asset Path: %ls", AssetPath.c_str());
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
	DataPath = RootPath / L"Data";
	AssetPath = RootPath / L"Asset";
	ShaderPath = AssetPath / L"Shader";
	CookedPath = DataPath/ L"Cooked";
	TextureCachePath = DataPath / L"TextureCache";
	AudioPath = AssetPath / "Audio";
	ScenePath = AssetPath / L"Scene";
	FontPath = AssetPath / "Font";
	LuaScriptPath = AssetPath / L"LuaScripts";
}

/**
 * @brief 경로 유효성을 확인하고 없는 디렉토리를 생성하는 함수
 */
void UPathManager::ValidateAndCreateDirectories() const
{
	TArray<path> DirectoriesToCreate = {
		DataPath,
		AssetPath,
		ShaderPath,
		CookedPath,
		TextureCachePath,
		AudioPath,
		ScenePath,
		FontPath,
		LuaScriptPath
	};

	for (const auto& Directory : DirectoriesToCreate)
	{
		try
		{
			if (!exists(Directory))
			{
				create_directories(Directory);
				UE_LOG("PathManager: Created Directory: %s", Directory.string().c_str());
			}
			else
			{
				UE_LOG("PathManager: Directory Exists: %s", Directory.string().c_str());
			}
		}
		catch (const filesystem::filesystem_error& e)
		{
			UE_LOG("PathManager:  Failed To Create Directory %s: %s", Directory.string().c_str(), e.what());
			assert(!"Asset 경로 생성 에러 발생");
		}
	}
}
