#include "pch.h"
#include "AnimSequenceViewerBootstrap.h"
#include "CameraActor.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "FViewport.h"
#include "FSkeletalViewerViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"

ViewerState* AnimSequenceViewerBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ViewerState* State = new ViewerState();
    State->Name = Name ? Name : "AnimSequenceViewer";

    // Preview world 생성
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);  // 메모리 최적화를 위한 프리뷰 월드
    State->World->Initialize();

    // World를 GEngine의 WorldContexts에 등록 (스키닝 모드 전환 등에서 접근 가능하도록)
    FWorldContext PreviewWorldContext(State->World, EWorldType::PreviewMinimal);
    GEngine.AddWorldContext(PreviewWorldContext);

    // 애니메이션 프리뷰용 쇼플래그 설정 (에디터 아이콘 비활성화)
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    // Viewport 생성
    State->Viewport = new FViewport();
    // 초기 크기는 매 프레임마다 조정됨
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);

    // FSkeletalViewerViewportClient 사용 (카메라 컨트롤 자동 활성화)
    auto* Client = new FSkeletalViewerViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);

    // 애니메이션 전신이 보이도록 카메라 위치 조정 (더 뒤로, 약간 위에서)
    Client->GetCamera()->SetActorLocation(FVector(5, 0, 1.5));

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);

    State->World->SetEditorCameraActor(Client->GetCamera());

    // 프리뷰용 SkeletalMeshActor 스폰
    if (State->World)
    {
        ASkeletalMeshActor* Preview = State->World->SpawnActor<ASkeletalMeshActor>();
        State->PreviewActor = Preview;

        if (Preview)
        {
            UE_LOG("[AnimSequenceViewerBootstrap] PreviewActor spawned successfully at (0,0,0)");
        }
        else
        {
            UE_LOG("[AnimSequenceViewerBootstrap] ERROR: Failed to spawn PreviewActor");
        }
    }

    return State;
}

void AnimSequenceViewerBootstrap::DestroyViewerState(ViewerState*& State)
{
    if (!State) return;

    // Viewport와 Client 정리
    if (State->Viewport) { delete State->Viewport; State->Viewport = nullptr; }
    if (State->Client) { delete State->Client; State->Client = nullptr; }

    // WorldContexts에서 제거 (World 삭제 전에 수행)
    if (State->World)
    {
        TArray<FWorldContext>& WorldContexts = const_cast<TArray<FWorldContext>&>(GEngine.GetWorldContexts());
        for (int i = WorldContexts.Num() - 1; i >= 0; --i)
        {
            if (WorldContexts[i].World == State->World)
            {
                WorldContexts.RemoveAt(i);
                break;
            }
        }
    }

    // World 정리 (내부적으로 모든 액터를 DestroyActor()로 정리함)
    if (State->World) { ObjectFactory::DeleteObject(State->World); State->World = nullptr; }

    // ViewerState 자체 삭제
    delete State;
    State = nullptr;
}
