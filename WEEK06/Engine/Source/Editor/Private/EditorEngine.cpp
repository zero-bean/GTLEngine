#include "pch.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_CLASS(UEditorEngine, UObject);

UEditorEngine* GEngine = nullptr;

UEditorEngine::UEditorEngine()
{
}

UEditorEngine::~UEditorEngine()
{
	Shutdown();
}

void UEditorEngine::Initialize()
{
	// Editor World 생성
	FWorldContext EditorContext;
	EditorContext.ContextHandle = FName("EditorWorld");
	EditorContext.WorldType = EWorldType::Editor;
	EditorContext.WorldPtr = NewObject<UWorld>();
	EditorContext.WorldPtr->SetWorldType(EWorldType::Editor);

	WorldContexts.push_back(EditorContext);

	// Editor 생성
	Editor = new UEditor();

	// 마지막 저장된 레벨 로드 (World에 위임)
	FString LastSavedLevelPath = UConfigManager::GetInstance().GetLastSavedLevelPath();
	if (!LoadLevel(LastSavedLevelPath))
	{
		CreateNewLevel();
	}
}

void UEditorEngine::Shutdown()
{
	// World 정리 (World 소멸자가 Level 정리)
	for (auto& Context : WorldContexts)
	{
		delete Context.WorldPtr;
	}
	WorldContexts.clear();

	// Editor 정리
	if (Editor)
	{
		delete Editor;
		Editor = nullptr;
	}
}

void UEditorEngine::BeginPlay()
{
}

void UEditorEngine::Tick(float DeltaSeconds)
{
	if (bPIEActive)
	{
		// PIE 중에는 PIEWorld만 Tick
		if (UWorld* PIEWorld = GetPIEWorld())
		{
			PIEWorld->Tick(DeltaSeconds);
		}
	}
	else
	{
		// 일반 에디터 모드에서는 EditorWorld만 Tick
		if (UWorld* EditorWorld = GetEditorWorld())
		{
			EditorWorld->Tick(DeltaSeconds);
		}
	}

	// Editor는 항상 Tick (Gizmo, Viewport 등)
	if (Editor)
	{
		Editor->Tick(DeltaSeconds);
	}
}

UWorld* UEditorEngine::GetEditorWorld() const
{
	for (const auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			return Context.WorldPtr;
		}
	}
	return nullptr;
}

UWorld* UEditorEngine::GetPIEWorld() const
{
	for (const auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return Context.WorldPtr;
		}
	}
	return nullptr;
}

UWorld* UEditorEngine::GetActiveWorld() const
{
	return bPIEActive ? GetPIEWorld() : GetEditorWorld();
}

bool UEditorEngine::LoadLevel(const FString& InFilePath)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	bool bSuccess = EditorWorld->LoadLevel(InFilePath);
	if (bSuccess)
	{
		UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);
	}
	return bSuccess;
}

bool UEditorEngine::SaveCurrentLevel(const FString& InFilePath)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	bool bSuccess = EditorWorld->SaveLevel(InFilePath);
	if (bSuccess)
	{
		UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);
	}
	return bSuccess;
}

bool UEditorEngine::CreateNewLevel(const FString& InLevelName)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	return EditorWorld->CreateNewLevel(InLevelName);
}

ULevel* UEditorEngine::GetCurrentLevel() const
{
	// PIE 모드일 때는 PIEWorld의 Level 반환
	if (bPIEActive)
	{
		if (UWorld* PIEWorld = GetPIEWorld())
		{
			return PIEWorld->GetLevel();
		}
	}

	// 에디터 모드일 때는 EditorWorld의 Level 반환
	if (UWorld* EditorWorld = GetEditorWorld())
	{
		return EditorWorld->GetLevel();
	}

	return nullptr;
}

void UEditorEngine::StartPIE()
{
	if (bPIEActive)
	{
		UE_LOG("EditorEngine: PIE is already running");
		return;
	}

	UE_LOG("EditorEngine: Starting Play In Editor (PIE) Mode");

	// PIE World 생성
	FWorldContext PIEContext;
	PIEContext.ContextHandle = FName("PIEWorld");
	PIEContext.WorldType = EWorldType::PIE;

	if (auto World = GetEditorWorld())
	{
		PIEContext.WorldPtr = DuplicateObject(World, World->GetOuter());
		PIEContext.WorldPtr->SetName("PIE World");
		PIEContext.WorldPtr->SetWorldType(EWorldType::PIE);
		PIEContext.WorldPtr->GetLevel()->SetName("PIE Level");
	}

	// WorldContexts에 추가
	WorldContexts.push_back(PIEContext);

	bPIEActive = true;

	for (auto& Context : WorldContexts)
	{
		UE_LOG("EditorEngine: WorldContext - Handle: %s, Type: %s", Context.ContextHandle.ToString().data(),
		       to_string(Context.WorldType).data());
		UE_LOG("Level Actor %llu", Context.World()->GetLevel()->GetActors().size());
	}

	// Editor World의 Level을 강제로 재초기화하여 렌더링 갱신
	if (UWorld* PIEWorld = GetPIEWorld())
	{
		if (ULevel* GetPIELevel = PIEWorld->GetLevel())
		{
			GetPIELevel->InitializeActorsInLevel();
			UE_LOG("EditorEngine: PIE Level reinitialized for rendering");
		}
	}

	UE_LOG("EditorEngine: PIE World created successfully");
}

void UEditorEngine::EndPIE()
{
	if (!bPIEActive)
	{
		UE_LOG("EditorEngine: PIE is not running");
		return;
	}

	UE_LOG("EditorEngine: Stopping Play In Editor (PIE) Mode");

	// bPIEActive를 먼저 false로 설정하여 Tick에서 EditorWorld로 전환
	bPIEActive = false;

	// PIE World Context 찾기 및 삭제
	for (auto It = WorldContexts.begin(); It != WorldContexts.end(); ++It)
	{
		if (It->WorldType == EWorldType::PIE)
		{
			// World 삭제
			delete It->WorldPtr;

			// Context 제거
			WorldContexts.erase(It);
			break;
		}
	}

	// Editor World의 Level을 강제로 재초기화하여 렌더링 갱신
	if (UWorld* EditorWorld = GetEditorWorld())
	{
		if (ULevel* EditorLevel = EditorWorld->GetLevel())
		{
			EditorLevel->InitializeActorsInLevel();
			UE_LOG("EditorEngine: Editor Level reinitialized for rendering");
		}
	}

	// Renderer의 Occlusion Culling 상태를 리셋하여 다음 프레임에서 제대로 렌더링되도록 함
	URenderer::GetInstance().ResetOcclusionCullingState();

	for (auto& Context : WorldContexts)
	{
		UE_LOG("EditorEngine: WorldContext - Handle: %s, Type: %s", Context.ContextHandle.ToString().data(),
		       to_string(Context.WorldType).data());
		UE_LOG("Level Actor %llu", Context.World()->GetLevel()->GetActors().size());
	}

	UE_LOG("EditorEngine: PIE World destroyed successfully");
	UE_LOG("EditorEngine: Switched back to Editor World");
}

UWorld* UEditorEngine::DuplicateWorldForPIE(UWorld* SourceWorld)
{
	return nullptr;
}

FWorldContext* UEditorEngine::GetEditorWorldContext()
{
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			return &Context;
		}
	}
	return nullptr;
}

FWorldContext* UEditorEngine::GetPIEWorldContext()
{
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return &Context;
		}
	}
	return nullptr;
}
