#include "pch.h"
#include "Manager/Level/Public/LevelManager.h"

#include "Level/Public/Level.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/SphereActor.h"
#include "Mesh/Public/TriangleActor.h"
#include "Mesh/Public/SquareActor.h"
#include "Manager/Path/Public/PathManager.h"
#include "Utility/Public/LevelSerializer.h"
#include "Utility/Public/Metadata.h"

IMPLEMENT_SINGLETON(ULevelManager)

ULevelManager::ULevelManager() = default;

ULevelManager::~ULevelManager() = default;

void ULevelManager::RegisterLevel(const FString& InName, ULevel* InLevel)
{
	Levels[InName] = InLevel;
	if (!CurrentLevel)
	{
		CurrentLevel = InLevel;
	}
}

void ULevelManager::LoadLevel(const FString& InName)
{
	if (Levels.find(InName) == Levels.end())
	{
		assert(!"Load할 레벨을 탐색하지 못함");
	}

	if (CurrentLevel)
	{
		CurrentLevel->Cleanup();
	}

	CurrentLevel = Levels[InName];

	CurrentLevel->Init();
}

void ULevelManager::Shutdown()
{
	for (auto& Level : Levels)
	{
		SafeDelete(Level.second);
	}
}

/**
 * @brief 기본 레벨을 생성하는 함수
 * XXX(KHJ): 이걸 지워야 할지, 아니면 Main Init에서만 배제할지 고민
 */
void ULevelManager::CreateDefaultLevel()
{
	Levels["Default"] = new ULevel("Default");
	LoadLevel("Default");
}

void ULevelManager::Update() const
{
	if (CurrentLevel)
	{
		CurrentLevel->Update();
	}
}

/**
 * @brief 현재 레벨을 지정된 경로에 저장
 */
bool ULevelManager::SaveCurrentLevel(const FString& InFilePath) const
{
	if (!CurrentLevel)
	{
		UE_LOG("[LevelManager] No Current Level To Save");
		return false;
	}

	// 기본 파일 경로 생성
	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		// 기본 파일명은 Level 이름으로 세팅
		FilePath = GenerateLevelFilePath(CurrentLevel->GetName().empty() ? "Untitled" : CurrentLevel->GetName());
	}

	UE_LOG("[LevelManager] Saving Current Level To: %s", FilePath.string().c_str());

	// LevelSerializer를 사용하여 저장
	try
	{
		// 현재 레벨의 메타데이터 생성
		FLevelMetadata Metadata = ConvertLevelToMetadata(CurrentLevel);

		bool bSuccess = FLevelSerializer::SaveLevelToFile(Metadata, FilePath.string());

		if (bSuccess)
		{
			UE_LOG("[LevelManager] Level Saved Successfully");
		}
		else
		{
			UE_LOG("[LevelManager] Failed To Save Level");
		}

		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("[LevelManager] Exception During Save: %s", Exception.what());
		return false;
	}
}

/**
 * @brief 지정된 파일로부터 Level Load & Register
 */
bool ULevelManager::LoadLevel(const FString& InLevelName, const FString& InFilePath)
{
	UE_LOG("[LevelManager] Loading Level '%s' From: %s", InLevelName.c_str(), InFilePath.c_str());

	// Make New Level
	ULevel* NewLevel = new ULevel(InLevelName);

	// 직접 LevelSerializer를 사용하여 로드
	try
	{
		FLevelMetadata Metadata;

		bool bLoadSuccess = FLevelSerializer::LoadLevelFromFile(Metadata, InFilePath);
		if (!bLoadSuccess)
		{
			UE_LOG("[LevelManager] Failed To Load Level From: %s", InFilePath.c_str());
			delete NewLevel;
			return false;
		}

		// 유효성 검사
		FString ErrorMessage;
		if (!FLevelSerializer::ValidateLevelData(Metadata, ErrorMessage))
		{
			UE_LOG("[LevelManager] Level Validation Failed: %s", ErrorMessage.c_str());
			delete NewLevel;
			return false;
		}

		// 메타데이터로부터 Level 생성
		bool bSuccess = LoadLevelFromMetadata(NewLevel, Metadata);

		if (!bSuccess)
		{
			UE_LOG("[LevelManager] Failed To Create Level From Metadata");
			delete NewLevel;
			return false;
		}
	}
	catch (const exception& InException)
	{
		UE_LOG("[LevelManager] Exception During Load: %s", InException.what());
		delete NewLevel;
		return false;
	}

	// 위에서 이미 로드 완료했으므로 Success 처리
	bool bSuccess = true;

	if (bSuccess)
	{
		// 기존 레벨이 있다면 정리
		ULevel* OldLevel = nullptr;

		if (Levels.find(InLevelName) != Levels.end())
		{
			OldLevel = Levels[InLevelName];

			// CurrentLevel이 삭제될 레벨과 같다면 미리 nullptr로 설정
			if (CurrentLevel == OldLevel)
			{
				CurrentLevel->Cleanup();
				CurrentLevel = nullptr;
			}

			delete OldLevel;
			Levels.erase(InLevelName);
		}

		// 새 레벨 등록 및 활성화
		RegisterLevel(InLevelName, NewLevel);

		// 현재 레벨을 로드된 레벨로 전환
		if (CurrentLevel && CurrentLevel != NewLevel)
		{
			CurrentLevel->Cleanup();
		}

		CurrentLevel = NewLevel;
		CurrentLevel->Init();

		UE_LOG("[LevelManager] Successfully Loaded And Switched To Level '%s'", InLevelName.c_str());
	}
	else
	{
		// 로드 실패 시 정리
		delete NewLevel;
		UE_LOG("[LevelManager] Failed To Load Level From File");
	}

	return bSuccess;
}

/**
 * @brief New Blank Level 생성
 */
bool ULevelManager::CreateNewLevel(const FString& InLevelName)
{
	UE_LOG("[LevelManager] Creating New Level: %s", InLevelName.c_str());

	// 이미 존재하는 레벨 이름인지 확인
	if (Levels.find(InLevelName) != Levels.end())
	{
		UE_LOG("[LevelManager] Level '%s' Already Exists", InLevelName.c_str());
		return false;
	}

	// 새 레벨 생성
	ULevel* NewLevel = new ULevel(InLevelName);

	// 레벨 등록 및 활성화
	RegisterLevel(InLevelName, NewLevel);

	// 현재 레벨을 새 레벨로 전환
	if (CurrentLevel && CurrentLevel != NewLevel)
	{
		CurrentLevel->Cleanup();
	}

	CurrentLevel = NewLevel;
	CurrentLevel->Init();

	UE_LOG("[LevelManager] Successfully created and switched to new level '%s'", InLevelName.c_str());

	return true;
}

/**
 * @brief 레벨 저장 디렉토리 경로 반환
 */
path ULevelManager::GetLevelDirectory() const
{
	UPathManager& PathManager = UPathManager::GetInstance();
	return PathManager.GetWorldPath();
}

/**
 * @brief 레벨 이름을 바탕으로 전체 파일 경로 생성
 */
path ULevelManager::GenerateLevelFilePath(const FString& InLevelName) const
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".json";
	path FullPath = LevelDirectory / FileName;
	return FullPath;
}

/**
 * @brief ULevel을 FLevelMetadata로 변환
 */
FLevelMetadata ULevelManager::ConvertLevelToMetadata(ULevel* InLevel) const
{
	FLevelMetadata Metadata;
	Metadata.Version = 1;
	Metadata.NextUUID = 1;

	if (!InLevel)
	{
		UE_LOG("[LevelManager] ConvertLevelToMetadata: Level Is Null");
		return Metadata;
	}

	// 레벨의 액터들을 순회하며 메타데이터로 변환
	uint32 CurrentID = 1;
	for (AActor* Actor : InLevel->GetLevelActors())
	{
		if (!Actor)
			continue;

		FPrimitiveMetadata PrimitiveMeta;
		PrimitiveMeta.ID = CurrentID++;
		PrimitiveMeta.Location = Actor->GetActorLocation();
		PrimitiveMeta.Rotation = Actor->GetActorRotation();
		PrimitiveMeta.Scale = Actor->GetActorScale3D();

		// Actor 타입에 따라 EPrimitiveType 설정
		if (dynamic_cast<ACubeActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Cube;
		}
		else if (dynamic_cast<ASphereActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Sphere;
		}
		// TODO(KHJ): TriangleActor 작성되면 추가할 것
		// else if (dynamic_cast<ATriangleActor*>(Actor))
		// {
		// 	PrimitiveMeta.Type = EPrimitiveType::Triangle;
		// }
		else
		{
			UE_LOG("[LevelManager] Unknown Actor Type, Skipping...");
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		Metadata.Primitives[PrimitiveMeta.ID] = PrimitiveMeta;
	}

	Metadata.NextUUID = CurrentID;

	UE_LOG("[LevelManager] Converted %zu Actors To Metadata", Metadata.Primitives.size());
	return Metadata;
}

/**
 * @brief FLevelMetadata로부터 ULevel에 Actor Load
 */
bool ULevelManager::LoadLevelFromMetadata(ULevel* InLevel, const FLevelMetadata& InMetadata) const
{
	if (!InLevel)
	{
		UE_LOG("[LevelManager] LoadLevelFromMetadata: InLevel Is Null");
		return false;
	}

	UE_LOG("[LevelManager] Loading %zu Primitives From Metadata", InMetadata.Primitives.size());

	// Metadata의 각 Primitive를 Actor로 생성
	for (const auto& [ID, PrimitiveMeta] : InMetadata.Primitives)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 적절한 액터 생성
		switch (PrimitiveMeta.Type)
		{
		case EPrimitiveType::Cube:
			NewActor = InLevel->SpawnActor<ACubeActor>();
			break;
		case EPrimitiveType::Sphere:
			NewActor = InLevel->SpawnActor<ASphereActor>();
			break;
		case EPrimitiveType::Triangle:
			NewActor = InLevel->SpawnActor<ATriangleActor>();
			break;
		case EPrimitiveType::Square:
			NewActor = InLevel->SpawnActor<ASquareActor>();
			break;
		// TODO(KHJ): TriangleActor 지원 예정
		// case EPrimitiveType::Triangle:
		// 	NewActor = InLevel->SpawnActor<ATriangleActor>();
		// 	break;
		default:
			UE_LOG("[LevelManager] Unknown Primitive Type: %d", static_cast<int32>(PrimitiveMeta.Type));
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		if (NewActor)
		{
			// Transform 정보 적용
			NewActor->SetActorLocation(PrimitiveMeta.Location);
			NewActor->SetActorRotation(PrimitiveMeta.Rotation);
			NewActor->SetActorScale3D(PrimitiveMeta.Scale);

			UE_LOG("[LevelManager] Created %s at (%.2f, %.2f, %.2f)", 
				FLevelSerializer::PrimitiveTypeToWideString(PrimitiveMeta.Type).c_str(),
				PrimitiveMeta.Location.X, PrimitiveMeta.Location.Y, PrimitiveMeta.Location.Z);
		}
		else
		{
			UE_LOG("[LevelManager] Failed To Create Actor For Primitive ID: %d", ID);
		}
	}

	UE_LOG("[LevelManager] Level Loaded From Metadata Successfully");
	// UE_LOG로 이미 처리됨
	return true;
}
