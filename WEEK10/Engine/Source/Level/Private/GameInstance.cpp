#include "pch.h"
#include "Level/Public/GameInstance.h"
#include "Level/Public/World.h"
#include "Core/Public/AppWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/GameViewportClient.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Manager/Lua/Public/LuaManager.h"

IMPLEMENT_CLASS(UGameInstance, UEngineSubsystem)

// Global GameInstance pointer
UGameInstance* GameInstance = nullptr;

UGameInstance::UGameInstance()
{
}

/**
 * @brief UEngineSubsystem 초기화
 */
void UGameInstance::Initialize()
{
	UEngineSubsystem::Initialize();
	GameInstance = this;

	// LuaManager 초기화
	ULuaManager::GetInstance().Initialize();

	UE_LOG_INFO("GameInstance: Subsystem initialized");
}

/**
 * @brief 게임 인스턴스 World 초기화 함수
 * StandAlone World와 Fullscreen Viewport 생성
 * @param InWindow 메인 윈도우
 * @param InScenePath 로드할 Scene 경로 (nullptr이면 빈 레벨 생성)
 */
void UGameInstance::InitializeWorld(FAppWindow* InWindow, const char* InScenePath)
{
	AppWindow = InWindow;

	// StandAlone World 생성 (PIE 타입)
	World = NewObject<UWorld>();
	World->SetWorldType(EWorldType::PIE);

	// GWorld 전역 포인터 설정 (Actor 초기화 시 필요)
	GWorld = World;

	// Scene 로드 또는 새 레벨 생성
	bool bLevelLoaded = false;
	FString ScenePathToLoad;

	if (InScenePath && strlen(InScenePath) > 0)
	{
		ScenePathToLoad = InScenePath;
	}
	else
	{
		// InScenePath가 없으면 Asset/Scene 폴더의 첫 번째 .Scene 파일 찾기
		const FString SceneFolder = "Asset/Scene";
		WIN32_FIND_DATAA FindData;
		HANDLE hFind = FindFirstFileA((SceneFolder + "/*.scene").c_str(), &FindData);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					ScenePathToLoad = SceneFolder + "/" + FindData.cFileName;
					UE_LOG_INFO("GameInstance: Found scene file '%s'", ScenePathToLoad.c_str());
					break;
				}
			} while (FindNextFileA(hFind, &FindData));

			FindClose(hFind);
		}

		if (ScenePathToLoad.empty())
		{
			UE_LOG_WARNING("GameInstance: No scene files found in '%s', creating new level", SceneFolder.c_str());
		}
	}

	// Scene 파일이 있으면 로드
	if (!ScenePathToLoad.empty())
	{
		if (!World->LoadLevel(ScenePathToLoad.c_str()))
		{
			UE_LOG_ERROR("GameInstance: Failed to load scene '%s', creating new level instead", ScenePathToLoad.c_str());
			World->CreateNewLevel();
		}
		else
		{
			UE_LOG_SUCCESS("GameInstance: Loaded scene '%s'", ScenePathToLoad.c_str());
			bLevelLoaded = true; // LoadLevel 내부에서 이미 BeginPlay 호출됨
		}
	}
	else
	{
		World->CreateNewLevel();
		UE_LOG_INFO("GameInstance: Created new level (no scene path provided)");
	}

	// LoadLevel이 성공하지 않은 경우에만 BeginPlay 호출 (LoadLevel 내부에서 이미 호출됨)
	if (!bLevelLoaded)
	{
		World->BeginPlay();
	}

	// 윈도우 크기 가져오기
	RECT ClientRect;
	GetClientRect(InWindow->GetWindowHandle(), &ClientRect);
	const uint32 Width = ClientRect.right - ClientRect.left;
	const uint32 Height = ClientRect.bottom - ClientRect.top;

	// StandAlone 모드: 마우스를 윈도우 영역에 제한하고 중앙으로 이동 (PIE처럼)
	POINT ClientTopLeft = { ClientRect.left, ClientRect.top };
	POINT ClientBottomRight = { ClientRect.right, ClientRect.bottom };
	ClientToScreen(InWindow->GetWindowHandle(), &ClientTopLeft);
	ClientToScreen(InWindow->GetWindowHandle(), &ClientBottomRight);

	RECT ClipRect;
	ClipRect.left = ClientTopLeft.x;
	ClipRect.top = ClientTopLeft.y;
	ClipRect.right = ClientBottomRight.x;
	ClipRect.bottom = ClientBottomRight.y;
	ClipCursor(&ClipRect);

	// 마우스 커서를 중앙으로 이동
	int32 CenterX = (ClipRect.left + ClipRect.right) / 2;
	int32 CenterY = (ClipRect.top + ClipRect.bottom) / 2;
	SetCursorPos(CenterX, CenterY);

	// 커서 숨김 (PIE와 동일한 패턴)
	while (ShowCursor(FALSE) >= 0) {}

	// 1. Fullscreen Viewport 생성 (캔버스)
	Viewport = new FViewport();
	Viewport->SetSize(Width, Height);
	Viewport->SetInitialPosition(0, 0);

	// 2. ViewportClient 생성 (화가)
	ViewportClient = NewObject<UGameViewportClient>();
	ViewportClient->SetViewportSize(FPoint(static_cast<int32>(Width), static_cast<int32>(Height)));

	// 3. ViewportClient에게 자신이 그릴 Viewport 참조 설정
	// Note: Unreal 패턴에서는 상호 참조지만, FViewport는 FViewportClient*만 받으므로
	// GameInstance가 둘 다 소유하고 ViewportClient만 Viewport 참조
	ViewportClient->SetOwningViewport(Viewport);

	UE_LOG_SUCCESS("GameInstance: Initialized world with fullscreen viewport (%dx%d)", Width, Height);
}

/**
 * @brief UEngineSubsystem 종료 함수
 * World와 Viewport 리소스 정리
 */
void UGameInstance::Deinitialize()
{
	// StandAlone 모드: 마우스 클리핑 해제 및 커서 복원
	ClipCursor(nullptr);
	while (ShowCursor(TRUE) < 0) {}

	// World 정리
	if (World)
	{
		World->EndPlay();
		delete World;
		World = nullptr;
	}

	// ViewportClient 정리 (화가)
	if (ViewportClient)
	{
		delete ViewportClient;
		ViewportClient = nullptr;
	}

	// Viewport 정리 (캔버스)
	if (Viewport)
	{
		delete Viewport;
		Viewport = nullptr;
	}

	GWorld = nullptr;
	GameInstance = nullptr;

	UEngineSubsystem::Deinitialize();
	UE_LOG_INFO("GameInstance: Deinitialize completed");
}

/**
 * @brief 매 프레임 업데이트
 * @param DeltaTime 프레임 시간
 */
void UGameInstance::Tick(float DeltaTime)
{
	// World Tick
	if (World)
	{
		World->Tick(DeltaTime);
	}

	// ViewportClient Tick
	if (ViewportClient)
	{
		ViewportClient->Tick();
	}
}

/**
 * @brief SceneView 생성
 * ViewportClient의 Transform 정보를 사용하여 렌더링용 SceneView 생성
 * @return 생성된 FSceneView 포인터 (호출자가 delete 필요)
 */
FSceneView* UGameInstance::CreateSceneView() const
{
	if (!ViewportClient || !Viewport || !World)
	{
		return nullptr;
	}

	// SceneView 생성
	FSceneView* SceneView = new FSceneView();

	// ViewportClient의 Transform으로 View/Projection 행렬 계산하여 초기화
	SceneView->InitializeWithMatrices(
		ViewportClient->GetViewMatrix(),
		ViewportClient->GetProjectionMatrix(),
		ViewportClient->GetViewLocation(),
		ViewportClient->GetViewRotation(),
		Viewport,
		World,
		EViewModeIndex::VMI_Gouraud,  // Default view mode
		ViewportClient->GetFOV(),
		ViewportClient->GetNearZ(),
		ViewportClient->GetFarZ()
	);

	return SceneView;
}
