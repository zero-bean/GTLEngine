#include "pch.h"
#include "Core/Public/Object.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Lua/Public/LuaManager.h"
#include "Manager/Sound/Public/SoundManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/PlayerCameraManager.h"

IMPLEMENT_CLASS(UEditorEngine, UObject)
UEditorEngine* GEditor = nullptr;
UWorld* GWorld = nullptr;
UEditorEngine::UEditorEngine()
{
    GEditor = this;
    UWorld* EditorWorld = NewObject<UWorld>(this);
    EditorWorld->SetWorldType(EWorldType::Editor);

    if (EditorWorld)
    {
        FWorldContext EditorContext;
        EditorContext.SetWorld(EditorWorld);
        WorldContexts.Add(EditorContext);

        GWorld = EditorWorld;
    }
    EditorModule = NewObject<UEditor>();

    CreateNewLevel();
    EditorWorld->BeginPlay();

    ULuaManager::GetInstance().Initialize();
}

UEditorEngine::~UEditorEngine()
{
    if (IsPIESessionActive())
    {
        EndPIE();
    }

    for (auto WorldContext : WorldContexts)
    {
        if (WorldContext.World())
        {
            delete WorldContext.World();
        }
    }
    if (EditorModule)
    {
        delete EditorModule;
    }

    GWorld = nullptr;
}

/**
 * @brief WorldContext를 순회하며 World의 Tick을 처리, EditorModule Update
 */
void UEditorEngine::Tick(float DeltaSeconds)
{
    for (FWorldContext& Context : WorldContexts)
    {
        UWorld* World = Context.World();
        if (World)
        {
            if (World->GetWorldType() == EWorldType::Editor)
            {
                World->Tick(DeltaSeconds);
            }
            else if (World->GetWorldType() == EWorldType::PIE)
            {
                // PIE 상태가 Playing일 때만 틱을 실행
                if (PIEState == EPIEState::Playing)
                {
                    World->Tick(DeltaSeconds);
                }
            }
        }
    }

    if (EditorModule)
    {
        EditorModule->Update();
    }

    ULuaManager::GetInstance().Update(DeltaSeconds);

    // Update audio system each frame
    USoundManager::GetInstance().Update(DeltaSeconds);

    // Deferred EndPIE
    if (bPendingEndPIE)
    {
        bPendingEndPIE = false;
        EndPIE();
    }
}

class UCamera* UEditorEngine::GetMainCamera() const
{
	int32 ActiveIdx = 0;
	if (GetPIEState() != EPIEState::Stopped)
	{
		ActiveIdx = UViewportManager::GetInstance().GetPIEActiveViewportIndex();
	}
	else
	{
		ActiveIdx = UViewportManager::GetInstance().GetActiveIndex();
	}

	return UViewportManager::GetInstance().GetClients()[ActiveIdx]->GetCamera();
}

/**
 * @brief PIE가 활성화되어 있는지 확인
 */
bool UEditorEngine::IsPIESessionActive() const
{
    for (const FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return true;
        }
    }
    return false;
}

/**
 * brief 에디터 월드를 복제해 PIE 시작
 */
void UEditorEngine::StartPIE()
{
    if (PIEState != EPIEState::Stopped)
    {
        return;
    }

    PIEState = EPIEState::Playing;
    // Ensure audio system is initialized as soon as PIE starts
    USoundManager::GetInstance().InitializeAudio();
    UWorld* EditorWorld = GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        return;
    }

    // PIE 시작 시 마지막으로 클릭한 뷰포트를 PIE 전용 뷰포트로 저장
    UViewportManager& ViewportMgr = UViewportManager::GetInstance();
    int32 LastClickedViewport = ViewportMgr.GetLastClickedViewportIndex();
    ViewportMgr.SetPIEActiveViewportIndex(LastClickedViewport);

    // PIE 뷰포트 중앙으로 마우스 이동 및 영역 제한
    if (LastClickedViewport >= 0 && LastClickedViewport < ViewportMgr.GetViewports().Num())
    {
        FViewport* PIEViewport = ViewportMgr.GetViewports()[LastClickedViewport];
        if (PIEViewport)
        {
            FRect ViewportRect = PIEViewport->GetRect();
            int32 CenterX = static_cast<int32>(ViewportRect.Left + ViewportRect.Width / 2);
            int32 CenterY = static_cast<int32>(ViewportRect.Top + ViewportRect.Height / 2);

            // Windows API로 마우스 커서 이동
            SetCursorPos(CenterX, CenterY);

            // 마우스를 뷰포트 영역에 제한
            RECT ClipRect;
            ClipRect.left = static_cast<LONG>(ViewportRect.Left);
            ClipRect.top = static_cast<LONG>(ViewportRect.Top);
            ClipRect.right = static_cast<LONG>(ViewportRect.Left + ViewportRect.Width);
            ClipRect.bottom = static_cast<LONG>(ViewportRect.Top + ViewportRect.Height);
            ClipCursor(&ClipRect);
        }
    }

    UWorld* PIEWorld = Cast<UWorld>(EditorWorld->Duplicate());

    if (PIEWorld)
    {
        PIEWorld->SetWorldType(EWorldType::PIE);
        FWorldContext PIEContext;
        PIEContext.SetWorld(PIEWorld);
        WorldContexts.Add(PIEContext);

        GWorld = PIEWorld;
        PIEWorld->BeginPlay();

        // WorldSettings 확인하여 Free Camera Mode vs Game Mode 결정
        const FWorldSettings& WorldSettings = PIEWorld->GetWorldSettings();
        if (WorldSettings.DefaultPlayerClass == nullptr)
        {
            // Free Camera Mode: 플레이어 없이 Editor Camera로 자유 비행
            UE_LOG_INFO("EditorEngine: PIE Free Camera Mode (No player spawned)");

            // Editor Camera PIE Free Camera Mode 활성화
            if (LastClickedViewport >= 0 && LastClickedViewport < ViewportMgr.GetViewports().Num())
            {
                FViewport* PIEViewport = ViewportMgr.GetViewports()[LastClickedViewport];
                if (PIEViewport)
                {
                    FViewportClient* ViewportClient = PIEViewport->GetViewportClient();
                    if (ViewportClient)
                    {
                        UCamera* EditorCamera = ViewportClient->GetCamera();
                        if (EditorCamera)
                        {
                            EditorCamera->SetPIEFreeCameraMode(true);
                            UE_LOG_INFO("EditorEngine: Editor Camera PIE Free Camera Mode enabled");
                        }
                    }
                }
            }
        }
        else
        {
            // Game Mode: 플레이어 스폰하고 PlayerCameraManager 사용 (기존 방식)
            UE_LOG_INFO("EditorEngine: PIE Game Mode (Player will be spawned)");
            InjectGameCameraIntoPIEViewport(PIEWorld, LastClickedViewport);
        }

        // PIE 시작 시 커서 숨김
        while (ShowCursor(FALSE) >= 0);
    }
}

/**
 * @brief PIE 종료 요청 (다음 프레임에 실행)
 * Tick이나 Lua 콜백 중에 호출해도 안전
 */
void UEditorEngine::RequestEndPIE()
{
    if (IsPIESessionActive())
    {
        UE_LOG("EditorEngine: RequestEndPIE - will end PIE at end of frame");
        bPendingEndPIE = true;
    }
}

/**
 * @brief PIE 종료하고 에디터 월드로 돌아감
 */
void UEditorEngine::EndPIE()
{
    if (PIEState == EPIEState::Stopped)
    {
        return;
    }
    PIEState = EPIEState::Stopped;

    // PIE Mouse Detach 상태 초기화
    bPIEMouseDetached = false;

    // Remove game camera from PIE viewport
    UViewportManager& ViewportMgr = UViewportManager::GetInstance();
    int32 PIEActiveViewport = ViewportMgr.GetPIEActiveViewportIndex();
    RemoveGameCameraFromPIEViewport(PIEActiveViewport);

    // PIE Free Camera Mode 비활성화
    if (PIEActiveViewport >= 0 && PIEActiveViewport < ViewportMgr.GetViewports().Num())
    {
        FViewport* PIEViewport = ViewportMgr.GetViewports()[PIEActiveViewport];
        if (PIEViewport)
        {
            FViewportClient* ViewportClient = PIEViewport->GetViewportClient();
            if (ViewportClient)
            {
                UCamera* EditorCamera = ViewportClient->GetCamera();
                if (EditorCamera && EditorCamera->IsPIEFreeCameraMode())
                {
                    EditorCamera->SetPIEFreeCameraMode(false);
                    UE_LOG_INFO("EditorEngine: Editor Camera PIE Free Camera Mode disabled");
                }
            }
        }
    }

    // PIE 전용 뷰포트 인덱스 리셋
    ViewportMgr.SetPIEActiveViewportIndex(-1);

    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        UWorld* PIEWorld = PIEContext->World();
        PIEWorld->EndPlay();
        delete PIEWorld;

        WorldContexts.Remove(*PIEContext);
    }

    // 오디오 정리: 모든 사운드 정지 후 오디오 시스템 종료
    USoundManager::GetInstance().StopAllSounds();
    USoundManager::GetInstance().ShutdownAudio();

    // GWorld를 다시 Editor World로 복원
    GWorld = GetEditorWorldContext().World();

    // 마우스 영역 제한 해제
    ClipCursor(nullptr);

    // PIE 종료 시 커서 복원
    while (ShowCursor(TRUE) < 0);
}

/**
 * @brief PIE 일시정지
 */
void UEditorEngine::PausePIE()
{
    if (PIEState != EPIEState::Playing)
    {
        return;
    }
    PIEState = EPIEState::Paused;
}

/**
 * @brief PIE 재개
 */
void UEditorEngine::ResumePIE()
{
    if (PIEState != EPIEState::Paused)
    {
        return;
    }
    PIEState = EPIEState::Playing;
}

/**
 * @brief 주어진 뷰포트 인덱스에 따라 렌더링할 World 반환
 * @param ViewportIndex 뷰포트 인덱스 (0~3)
 * @return PIE active viewport면 PIE World, 아니면 Editor World
 */
UWorld* UEditorEngine::GetWorldForViewport(int32 ViewportIndex)
{
    // PIE가 활성화되지 않았으면 항상 에디터 월드
    if (!IsPIESessionActive())
    {
        return GetEditorWorldContext().World();
    }

    // PIE 활성 뷰포트 확인
    int32 PIEActiveIndex = UViewportManager::GetInstance().GetPIEActiveViewportIndex();

    // 현재 뷰포트가 PIE 활성 뷰포트면 PIE World, 아니면 Editor World
    if (ViewportIndex == PIEActiveIndex)
    {
        FWorldContext* PIEContext = GetPIEWorldContext();
        return PIEContext ? PIEContext->World() : GetEditorWorldContext().World();
    }
    else
    {
        return GetEditorWorldContext().World();
    }
}

/**
 * @brief 경로의 파일을 불러와서 현재 Editor 월드의 Level 교체
 */
bool UEditorEngine::LoadLevel(const FString& InFilePath)
{
    UE_LOG("GEditor: Loading Level: %s", InFilePath.data());

    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive())
    {
        EndPIE();
    }
    return GetEditorWorldContext().World()->LoadLevel(path(InFilePath));
}

/**
 * @brief 현재 Editor 월드의 레벨을 파일로 저장
 */
bool UEditorEngine::SaveCurrentLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Saving Level: %s", InLevelName.c_str());

    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive())
    {
        EndPIE();
    }

    path FilePath = InLevelName;
    if (FilePath.empty())
    {
        FName CurrentLevelName = GetEditorWorldContext().World()->GetLevel()->GetName();
        FilePath = GenerateLevelFilePath(CurrentLevelName == FName::GetNone()? "Untitled" : CurrentLevelName.ToString());
    }

    UE_LOG("GEditor: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

    try
    {
        bool bSuccess = GetEditorWorldContext().World()->SaveCurrentLevel(FilePath);
        if (bSuccess)
        {
            UE_LOG("GEditor: 레벨이 성공적으로 저장되었습니다");
        }
        else
        {
            UE_LOG("GEditor: 레벨을 저장하는 데에 실패했습니다");
        }

        return bSuccess;
    }
    catch (const exception& Exception)
    {
        UE_LOG("GEditor: 저장 과정에서 Exception 발생: %s", Exception.what());
        return false;
    }
}

/**
 * @brief 현재 Editor 월드에 새 레벨 변경
 */
bool UEditorEngine::CreateNewLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Create New Level: %s", InLevelName.c_str());

    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive()) { EndPIE(); }
    GetEditorWorldContext().World()->CreateNewLevel(InLevelName);
    return true;
}

path UEditorEngine::GenerateLevelFilePath(const FString& InLevelName)
{
    path LevelDirectory = GetLevelDirectory();
    path FileName = InLevelName + ".scene";
    return LevelDirectory / FileName;
}

path UEditorEngine::GetLevelDirectory()
{
    UPathManager& PathManager = UPathManager::GetInstance();
    return PathManager.GetScenePath();
}

FWorldContext* UEditorEngine::GetPIEWorldContext()
{
    for (FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return &Context;
        }
    }
    return nullptr;
}

FWorldContext* UEditorEngine::GetActiveWorldContext()
{
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        return PIEContext;
    }

    if (!WorldContexts.IsEmpty())
    {
        return &WorldContexts[0];
    }

    return nullptr;
}

/**
 * @brief PIE 시작 시 PlayerCameraManager를 뷰포트에 설정
 */
void UEditorEngine::InjectGameCameraIntoPIEViewport(UWorld* PIEWorld, int32 ViewportIndex)
{
    if (!PIEWorld)
    {
        return;
    }

    // Get GameMode
    AGameMode* GameMode = PIEWorld->GetGameMode();
    if (!GameMode)
    {
        UE_LOG_WARNING("EditorEngine: No GameMode found in PIE world, using editor camera");
        return;
    }

    // Get PlayerCameraManager
    APlayerCameraManager* CameraManager = GameMode->GetPlayerCameraManager();
    if (!CameraManager)
    {
        UE_LOG_WARNING("EditorEngine: No PlayerCameraManager found, using editor camera");
        return;
    }

    // Get viewport and inject PlayerCameraManager
    UViewportManager& ViewportMgr = UViewportManager::GetInstance();
    if (ViewportIndex < 0 || ViewportIndex >= ViewportMgr.GetViewports().Num())
    {
        return;
    }

    FViewport* PIEViewport = ViewportMgr.GetViewports()[ViewportIndex];
    if (!PIEViewport)
    {
        return;
    }

    FViewportClient* ViewportClient = PIEViewport->GetViewportClient();
    if (!ViewportClient)
    {
        return;
    }

    // Set PlayerCameraManager for PIE viewport
    ViewportClient->SetPlayerCameraManager(CameraManager);
    UE_LOG_INFO("EditorEngine: PlayerCameraManager set for PIE viewport");
}

/**
 * @brief PIE 종료 시 PlayerCameraManager를 뷰포트에서 제거
 */
void UEditorEngine::RemoveGameCameraFromPIEViewport(int32 ViewportIndex)
{
    UViewportManager& ViewportMgr = UViewportManager::GetInstance();

    if (ViewportIndex < 0 || ViewportIndex >= ViewportMgr.GetViewports().Num())
    {
        return;
    }

    FViewport* PIEViewport = ViewportMgr.GetViewports()[ViewportIndex];
    if (!PIEViewport)
    {
        return;
    }

    FViewportClient* ViewportClient = PIEViewport->GetViewportClient();
    if (!ViewportClient)
    {
        return;
    }

    // Remove PlayerCameraManager, revert to editor camera
    ViewportClient->SetPlayerCameraManager(nullptr);
    UE_LOG_INFO("EditorEngine: PlayerCameraManager removed from PIE viewport");
}

/**
 * @brief PIE 중 마우스 입력 분리 / 재부착 토글 (Shift + F1)
 * PIE 중에만 작동하며, 마우스 입력을 게임에서 분리하여 에디터 UI를 조작 가능하게 함
 */
void UEditorEngine::TogglePIEMouseDetach()
{
    if (!IsPIESessionActive())
    {
        return;
    }

    bPIEMouseDetached = !bPIEMouseDetached;

    // PIE World의 입력 차단 플래그 설정
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext && PIEContext->World())
    {
        PIEContext->World()->SetIgnoreInput(bPIEMouseDetached);
    }

    if (bPIEMouseDetached)
    {
        UE_LOG_INFO("PIE: Mouse Input Detached (Shift+F1)");
        // 마우스 영역 제한 해제
        ClipCursor(nullptr);
        // 커서 표시
        while (ShowCursor(TRUE) < 0);
    }
    else
    {
        UE_LOG_INFO("PIE: Mouse Input Reattached (Shift+F1)");
        // 마우스를 뷰포트 영역에 다시 제한
        UViewportManager& ViewportMgr = UViewportManager::GetInstance();
        int32 PIEActiveViewport = ViewportMgr.GetPIEActiveViewportIndex();
        if (PIEActiveViewport >= 0 && PIEActiveViewport < ViewportMgr.GetViewports().Num())
        {
            FViewport* PIEViewport = ViewportMgr.GetViewports()[PIEActiveViewport];
            if (PIEViewport)
            {
                FRect ViewportRect = PIEViewport->GetRect();
                RECT ClipRect;
                ClipRect.left = static_cast<LONG>(ViewportRect.Left);
                ClipRect.top = static_cast<LONG>(ViewportRect.Top);
                ClipRect.right = static_cast<LONG>(ViewportRect.Left + ViewportRect.Width);
                ClipRect.bottom = static_cast<LONG>(ViewportRect.Top + ViewportRect.Height);
                ClipCursor(&ClipRect);
            }
        }
        // 커서 숨김
        while (ShowCursor(FALSE) >= 0);
    }
}
