#include "pch.h"
#include "ParticleEditorBootstrap.h"
#include "ParticleEditorState.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Grid/GridActor.h"

ParticleEditorState* ParticleEditorBootstrap::CreateEditorState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ParticleEditorState* State = new ParticleEditorState();
    State->Name = Name ? Name : "Particle Editor";

    // Preview world 만들기
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);  // Set as preview world for memory optimization
    State->World->Initialize();
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    State->World->GetGizmoActor()->SetSpace(EGizmoSpace::Local);

    // Viewport + client per tab
    State->Viewport = new FViewport();
    // 프레임 마다 initial size가 바꿜 것이다
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);

    // Create viewport client
    FViewportClient* Client = new FViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);
    Client->SetViewportBackgroundColor(FVector4(
        State->ViewportBackgroundColor[0],
        State->ViewportBackgroundColor[1],
        State->ViewportBackgroundColor[2],
        State->ViewportBackgroundColor[3]));

    // Set camera location after client is set up
    if (Client->GetCamera())
    {
        Client->GetCamera()->SetActorLocation(FVector(-3.0f, -3.0f, 1.0f));
        Client->GetCamera()->SetRotationFromEulerAngles(FVector(-30.f, 0.0f, 45.0f));
    }

    State->World->SetEditorCameraActor(Client->GetCamera());

    // TODO(PYB): AParticleSystemActor 기능이 구현되면 주석 해제할 것
    // if (State->World)
    // {
    //     AParticleSystemActor* Preview = State->World->SpawnActor<AParticleSystemActor>();
    //     State->PreviewActor = Preview;
    // }

    if (InWorld)
    {
        State->World->GetRenderSettings().SetShowFlags(InWorld->GetRenderSettings().GetShowFlags());
        State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);
    }

    if (State->World && State->World->GetGridActor())
    {
        State->World->GetGridActor()->SetGridVisible(State->bShowGrid);
        State->World->GetGridActor()->SetAxisVisible(State->bShowAxis);
    }

    return State;
}

void ParticleEditorBootstrap::DestroyEditorState(ParticleEditorState*& State)
{
    if (!State) return;
    if (State->Viewport) { delete State->Viewport; State->Viewport = nullptr; }
    if (State->Client) { delete State->Client; State->Client = nullptr; }
    if (State->World) { ObjectFactory::DeleteObject(State->World); State->World = nullptr; }
    delete State; State = nullptr;
}
