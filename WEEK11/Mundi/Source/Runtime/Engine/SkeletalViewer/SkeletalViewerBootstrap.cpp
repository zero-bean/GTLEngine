#include "pch.h"
#include "SkeletalViewerBootstrap.h"
#include "CameraActor.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "FViewport.h"
#include "FSkeletalViewerViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"

ViewerState* SkeletalViewerBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ViewerState* State = new ViewerState();
    State->Name = Name ? Name : "Viewer";

    // Preview world 만들기
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);  // Set as preview world for memory optimization
    State->World->Initialize();

    // World를 GEngine의 WorldContexts에 등록 (스키닝 모드 전환 등에서 접근 가능하도록)
    FWorldContext PreviewWorldContext(State->World, EWorldType::PreviewMinimal);
    GEngine.AddWorldContext(PreviewWorldContext);

    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    State->World->GetGizmoActor()->SetSpace(EGizmoSpace::Local);
    
    // Viewport + client per tab
    State->Viewport = new FViewport();
    // 프레임 마다 initial size가 바꿜 것이다
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);

    auto* Client = new FSkeletalViewerViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);
    Client->GetCamera()->SetActorLocation(FVector(3, 0, 2));

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);

    State->World->SetEditorCameraActor(Client->GetCamera());

    // Spawn a persistent preview actor (mesh can be set later from UI)
    if (State->World)
    {
        ASkeletalMeshActor* Preview = State->World->SpawnActor<ASkeletalMeshActor>();
        State->PreviewActor = Preview;
    }

    return State;
}

void SkeletalViewerBootstrap::DestroyViewerState(ViewerState*& State)
{
    if (!State) return;
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

    if (State->World) { ObjectFactory::DeleteObject(State->World); State->World = nullptr; }
    delete State; State = nullptr;
}
