#include "pch.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/AmbientLight.h"
#include "Actor/Public/GameMode.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
	: WorldType(EWorldType::Editor)
	, bBegunPlay(false)
{
}

UWorld::UWorld(EWorldType InWorldType)
	: WorldType(InWorldType)
	, bBegunPlay(false)
{
}

UWorld::~UWorld()
{
	EndPlay();
	if (Level)
	{
		ULevel* CurrentLevel = Level;
		SafeDelete(CurrentLevel); // 내부 Clean up은 Level의 소멸자에서 수행
		Level = nullptr;
	}
}

void UWorld::BeginPlay()
{
	if (bBegunPlay)
	{
		return;
	}

	if (!Level)
	{
		UE_LOG_ERROR("World: BeginPlay 호출 전에 로드된 Level이 없습니다.");
		return;
	}

	// GameMode는 오직 PIE 월드나 Game 월드에서만 스폰
	if (WorldType == EWorldType::PIE || WorldType == EWorldType::Game)
	{
		if (AuthorityGameMode == nullptr)
		{
			AuthorityGameMode = Cast<AGameMode>(SpawnActor(AGameMode::StaticClass()));
			AuthorityGameMode->BeginPlay();
		}
	}

	Level->Init();
	bBegunPlay = true;

	// CinematicManager가 레벨에 있는지 확인
	bool bHasCinematicManager = false;
	if (Level)
	{
		TArray<AActor*> AllActors = Level->GetLevelActors();
		for (AActor* Actor : AllActors)
		{
			if (Actor && Actor->GetName().ToString() == "CinematicManager")
			{
				bHasCinematicManager = true;
				UE_LOG_INFO("World: CinematicManager found, delaying StartGame()");
				break;
			}
		}
	}

	// CinematicManager가 없는 경우에만 즉시 게임 시작
	// CinematicManager가 있으면 코루틴 종료 후 StartGame() 호출
	if (AuthorityGameMode && !bHasCinematicManager)
	{
		UE_LOG_INFO("World: All actors initialized, calling GameMode::StartGame()");
		AuthorityGameMode->StartGame();
	}
}

bool UWorld::EndPlay()
{
	if (!Level || !bBegunPlay)
	{
		bBegunPlay = false;
		return false;
	}

	FlushPendingDestroy();
	// Level EndPlay
	bBegunPlay = false;
	return true;
}

void UWorld::Tick(float DeltaTimes)
{
	WorldTimeSeconds += DeltaTimes;
	FrameNumber++;
	if (!Level || !bBegunPlay)
	{
		return;
	}

	// 스폰 / 삭제 처리
	FlushPendingDestroy();

	// TODO: 현재 임시로 OCtree 업데이트 처리
	Level->UpdateOctree();

	if (WorldType == EWorldType::Editor )
	{
		// 액터 배열 복사본으로 순회 (Tick 도중 액터가 추가/삭제될 수 있음)
		TArray<AActor*> ActorsToTick = Level->GetLevelActors();
		for (AActor* Actor : ActorsToTick)
		{
			if(Actor->CanTickInEditor() && Actor->CanTick())
			{
				Actor->Tick(DeltaTimes);
			}

			if (Actor->IsPendingDestroy())
			{
				DestroyActor(Actor);
			}
		}
	}

	if (WorldType == EWorldType::Game || WorldType == EWorldType::PIE)
	{
		// 중앙집중식 overlap 업데이트 (Unreal Engine 방식)
		// PIE/Game 모드에서만 실행 (Editor 모드에서는 실행 안 함)
		// Component tick 여부와 무관하게 모든 overlap을 한번에 체크
		Level->UpdateAllOverlaps();

		// 액터 배열 복사본으로 순회 (Tick 도중 액터가 추가/삭제될 수 있음)
		TArray<AActor*> ActorsToTick = Level->GetLevelActors();
		for (AActor* Actor : ActorsToTick)
		{
			if(Actor->CanTick())
			{
				Actor->Tick(DeltaTimes);
			}

			if (Actor->IsPendingDestroy())
			{
				DestroyActor(Actor);
			}
		}
	}
}

ULevel* UWorld::GetLevel() const
{
	return Level;
}

/**
* @brief 지정된 경로에서 Level을 로드하고 현재 Level로 전환합니다.
* @param InLevelFilePath 로드할 Level 파일 경로
* @return 로드 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::LoadLevel(path InLevelFilePath)
{
	JSON LevelJson;
	ULevel* NewLevel = nullptr;

	try
	{
		FString LevelNameString = InLevelFilePath.stem().string();
		NewLevel = NewObject<ULevel>(this);
		NewLevel->SetName(LevelNameString);

		if (!FJsonSerializer::LoadJsonFromFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level JSON 로드에 실패했습니다: %s", InLevelFilePath.string().c_str());
			SafeDelete(NewLevel);
			return false;
		}

		NewLevel->SetOuter(this);
		SwitchToLevel(NewLevel);
		NewLevel->Serialize(true, LevelJson);

		BeginPlay();

		// 레벨 로드 완료 후 Octree 전체 구축
		NewLevel->UpdateOctreeImmediate();

		// Notify listeners that level has changed
		OnLevelChanged.Broadcast();
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 로드 중 예외 발생: %s", Exception.what());
		SafeDelete(NewLevel);
		CreateNewLevel();
		BeginPlay();
		return false;
	}


	return true;
}

/**
* @brief 현재 Level을 지정된 경로에 저장합니다.
* @param InLevelFilePath 저장할 파일 경로
* @return 저장 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::SaveCurrentLevel(path InLevelFilePath) const
{
	if (!Level)
	{
		UE_LOG_ERROR("World: 저장할 Level이 없습니다.");
		return false;
	}

	if(WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
	{
		UE_LOG_ERROR("World: 게임 또는 PIE 모드에서는 Level 저장이 허용되지 않습니다.");
		return false;
	}

	try
	{
		JSON LevelJson;
		Level->Serialize(false, LevelJson);

		if (!FJsonSerializer::SaveJsonToFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level 저장에 실패했습니다: %s", InLevelFilePath.string().c_str());
			return false;
		}

	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 저장 중 예외 발생: %s", Exception.what());
		return false;
	}

	return true;
}

AActor* UWorld::SpawnActor(UClass* InActorClass, JSON* ActorJsonData)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Actor를 Spawn할 수 있는 Level이 없습니다.");
		return nullptr;
	}

	if (!InActorClass)
	{
		return nullptr;
	}

	// TODO: 리팩토링 필요
	// - Level->SpawnActorToLevel()로 위임하여 중복 코드 제거
	// - Level의 private 멤버(LevelActors) 직접 접근 문제
	// - bBegunPlay 체크를 파라미터로 전달하는 방식 검토
	// FIX: Actor's Outer should be Level, not World (for GetOuter() to work correctly)
	AActor* NewActor = Cast<AActor>(NewObject(InActorClass, Level));
	if (NewActor)
	{
		Level->LevelActors.Add(NewActor);
		if (ActorJsonData != nullptr)
		{
			NewActor->Serialize(true, *ActorJsonData);
		}
		else
		{
			NewActor->InitializeComponents();
		}

		if (bBegunPlay)
		{
			NewActor->BeginPlay();
		}
		Level->AddLevelComponent(NewActor);

		// 템플릿 액터면 캐시에 추가
		if (NewActor->IsTemplate())
		{
			Level->RegisterTemplateActor(NewActor);
		}

		return NewActor;
	}

	return nullptr;
}

AActor* UWorld::SpawnActor(const std::string& ClassName)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Actor를 Spawn할 수 있는 Level이 없습니다.");
		return nullptr;
	}

	UClass* ActorClass = UClass::FindClass(FName(ClassName));
	if (!ActorClass)
	{
		UE_LOG_ERROR("World: SpawnActor - Class not found: %s", ClassName.c_str());
		return nullptr;
	}

	return Level->SpawnActorToLevel(ActorClass);
}

/**
* @brief 지정된 Actor를 월드에서 삭제합니다. 실제 삭제는 안전한 시점에 이루어집니다.
* @param InActor 삭제할 Actor
* @return 삭제 요청이 성공적으로 접수되었는지 여부
*/
bool UWorld::DestroyActor(AActor* InActor)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Level이 없어 Actor 삭제를 수행할 수 없습니다.");
		return false;
	}

	if (!InActor)
	{
		UE_LOG_ERROR("World: DestroyActor에 null 포인터가 전달되었습니다.");
		return false;
	}

	if (std::find(PendingDestroyActors.begin(), PendingDestroyActors.end(), InActor) != PendingDestroyActors.end())
	{
		UE_LOG_ERROR("World: 이미 삭제 대기 중인 액터에 대한 중복 삭제 요청입니다.");
		return false; // 이미 삭제 대기 중인 액터
	}

	PendingDestroyActors.Add(InActor);
	return true;
}

TArray<AActor*> UWorld::FindActorsOfClass(UClass* InClass) const
{
	TArray<AActor*> Result;

	if (!Level || !InClass)
	{
		return Result;
	}

	const TArray<AActor*>& AllActors = Level->GetLevelActors(); // <--- ULevel에 이 함수가 필요합니다.

	for (AActor* Actor : AllActors)
	{
		if (Actor && Actor->GetClass()->IsChildOf(InClass))
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

EWorldType UWorld::GetWorldType() const
{
	return WorldType;
}

void UWorld::SetWorldType(EWorldType InWorldType)
{
	WorldType = InWorldType;
}

/**
 * @brief 삭제 대기 중인 Actor들을 실제로 삭제합니다.
 * @note 이 함수는 Tick 루프 내에서 안전한 시점에 호출되어야 합니다.
 */
void UWorld::FlushPendingDestroy()
{
	if (PendingDestroyActors.IsEmpty() || !Level)
	{
		return;
	}

	TArray<AActor*> ActorsToProcess = PendingDestroyActors;
	PendingDestroyActors.Empty();
	UE_LOG("World: %d개의 Actor를 삭제합니다.", ActorsToProcess.Num());
	for (AActor* ActorToDestroy : ActorsToProcess)
	{
		if (!Level->DestroyActor(ActorToDestroy))
		{
			UE_LOG_ERROR("World: Actor 삭제에 실패했습니다: %s", ActorToDestroy->GetName().ToString().c_str());
		}
	}
}

/**
 * @brief 현재 Level을 새 Level로 전환합니다. 기존 Level은 소멸됩니다.
 * @param InNewLevel 새로 전환할 Level
 * @note 이전 Level의 안전한 종료 및 메모리 해제를 여기에서 책입집니다.
 */
void UWorld::SwitchToLevel(ULevel* InNewLevel)
{
	EndPlay();
	if (Level)
	{
		ULevel* OldLevel = Level;
		SafeDelete(OldLevel);
		Level = nullptr;
	}

	Level = InNewLevel;
	if (Level)
	{
		Level->OwningWorld = this;
	}

	PendingDestroyActors.Empty();
	bBegunPlay = false;
}

UObject* UWorld::Duplicate()
{
	UWorld* World = Cast<UWorld>(Super::Duplicate());
	World->Settings = Settings;

	// PIE World가 어느 Editor World로부터 복제되었는지 추적
	World->SetSourceEditorWorld(this);

	return World;
}

void UWorld::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	UWorld* World = Cast<UWorld>(DuplicatedObject);
	World->Level = Cast<ULevel>(Level->Duplicate());
	World->Level->SetOuter(World);  // Level의 Outer를 World로 설정
	World->Level->OwningWorld = World;  // Level이 자신을 소유한 World를 알도록 설정
}

void UWorld::CreateNewLevel(const FName& InLevelName)
{
	ULevel* NewLevel = NewObject<ULevel>();
	NewLevel->SetName(InLevelName);
	NewLevel->SetOuter(this);
	SwitchToLevel(NewLevel);

	// 기본 AmbientLight 추가
	AActor* SpawnedActor = SpawnActor(AAmbientLight::StaticClass());
	if (AAmbientLight* AmbientLight = Cast<AAmbientLight>(SpawnedActor))
	{
		AmbientLight->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		AmbientLight->SetName("AmbientLight");
	}

	BeginPlay();

	// 새 레벨 생성 후 Octree 전체 구축
	NewLevel->UpdateOctreeImmediate();

	// Notify listeners that level has changed
	OnLevelChanged.Broadcast();
}

AActor* UWorld::FindTemplateActorOfName(const std::string& InName)
{
	// PIE World인 경우 원본 Editor World에서 template actor 검색
	if (SourceEditorWorld != nullptr)
	{
		ULevel* EditorLevel = SourceEditorWorld->GetLevel();
		if (!EditorLevel)
		{
			UE_LOG_ERROR("World: SourceEditorWorld의 Level이 없어 template actor를 검색할 수 없습니다.");
			return nullptr;
		}
		return EditorLevel->FindTemplateActorByName(FName(InName));
	}

	// Editor/Game World인 경우 자신의 Level에서 검색
	if (!Level)
	{
		UE_LOG_ERROR("World: Level이 없어 template actor를 검색할 수 없습니다.");
		return nullptr;
	}

	return Level->FindTemplateActorByName(FName(InName));
}
