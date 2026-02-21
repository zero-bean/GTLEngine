#pragma once
#include "Core/Public/Object.h"

UCLASS()
class UPathManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UPathManager, UObject)

public:
	void Init();

	// Base Path
	const path& GetRootPath() const { return RootPath; }
	const path& GetDataPath() const { return DataPath; }
	const path& GetAssetPath() const { return AssetPath; }

	// Detailed Asset Path
	const path& GetShaderPath() const { return ShaderPath; }
	const path& GetCookedPath() const { return CookedPath; }
	const path& GetTextureCachePath() const { return TextureCachePath; }
	const path& GetAudioPath() const { return AudioPath; }
	const path& GetScenePath() const { return ScenePath; }
	const path& GetFontPath() const { return FontPath; }
	const path& GetLuaScriptPath() const { return LuaScriptPath; }

private:
	path RootPath;
	path DataPath;
	path AssetPath;
	path ShaderPath;
	path CookedPath;
	path TextureCachePath;
	path AudioPath;
	path ScenePath;
	path FontPath;
	path LuaScriptPath;

	void InitializeRootPath();
	void GetEssentialPath();
	void ValidateAndCreateDirectories() const;
};
