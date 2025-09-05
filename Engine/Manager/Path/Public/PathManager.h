#pragma once
#include "Core/Public/Object.h"

class UPathManager :
	public UObject
{
	DECLARE_SINGLETON(UPathManager)

public:
	void Init();

	// Base Path
	const path& GetRootPath() const { return RootPath; }
	const path& GetAssetPath() const { return AssetPath; }

	// Detailed Asset Path
	const path& GetShaderPath() const { return ShaderPath; }
	const path& GetTexturePath() const { return TexturePath; }
	const path& GetModelPath() const { return ModelPath; }
	const path& GetAudioPath() const { return AudioPath; }
	const path& GetWorldPath() const { return WorldPath; }
	const path& GetConfigPath() const { return ConfigPath; }

private:
	path RootPath;
	path AssetPath;
	path ShaderPath;
	path TexturePath;
	path ModelPath;
	path AudioPath;
	path WorldPath;
	path ConfigPath;

	void InitializeRootPath();
	void GetEssentialPath();
	void ValidateAndCreateDirectories() const;
};

// path SolutionPath;
// path ProjectPath;
// path EnginePath;
// path ExternalPath;
//
// // Asset 관련 경로들
//


// const path& GetProjectPath() const { return ProjectPath; }
// const path& GetEnginePath() const { return EnginePath; }
// const path& GetExternalPath() const { return ExternalPath; }
//
// // Asset 관련 경로들
// const path& GetTexturePath() const { return TexturePath; }
// const path& GetModelPath() const { return ModelPath; }
// const path& GetAudioPath() const { return AudioPath; }
// const path& GetScenePath() const { return ScenePath; }
// const path& GetConfigPath() const { return ConfigPath; }
//
// // 유틸리티 함수들
// string GetRelativeAssetPath(const path& AssetFilePath) const;
// string GetAbsoluteAssetPath(const string& RelativeAssetPath) const;
// bool IsValidAssetPath(const path& AssetFilePath) const;
// bool CreateAssetDirectories() const;
//
// // Asset 타입별 경로 반환
