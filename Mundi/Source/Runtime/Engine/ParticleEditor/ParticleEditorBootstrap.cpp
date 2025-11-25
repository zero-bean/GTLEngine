#include "pch.h"
#include "ParticleEditorBootstrap.h"
#include "ParticleEditorState.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Grid/GridActor.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule/ParticleModuleRequired.h"
#include "ParticleModule/ParticleModuleSpawn.h"
#include "ParticleModule/ParticleModuleLifetime.h"
#include "ParticleModule/ParticleModuleSize.h"
#include "ParticleModule/ParticleModuleColor.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"

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

    // 언리얼 방식: 빈 파티클 시스템에 기본 Sprite Emitter 1개 자동 생성
    State->CurrentParticleSystem = NewObject<UParticleSystem>();
    State->LoadedParticleSystemPath = "Untitled Particle System";

    // 기본 Sprite Emitter 생성
    UParticleEmitter* DefaultEmitter = NewObject<UParticleEmitter>();

    // LOD Level 0 생성
    UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
    LODLevel->Level = 0;
    LODLevel->bEnabled = true;
    DefaultEmitter->LODLevels.Add(LODLevel);

    // Required Module (필수)
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    Required->Material = UResourceManager::GetInstance().Load<UMaterial>("Shaders/UI/Billboard.hlsl");
    Required->EmitterDuration = 2.0f;
    Required->EmitterLoops = 0;
    LODLevel->RequiredModule = Required;

    // Spawn Module (필수)
    UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
    SpawnModule->SpawnRate.Operation = EDistributionMode::DOP_Constant;
    SpawnModule->SpawnRate.Constant = 30.0f;
    LODLevel->SpawnModule = SpawnModule;

    // 기본 모듈들 추가
    UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
    LifetimeModule->LifeTime.Operation = EDistributionMode::DOP_Uniform;
    LifetimeModule->LifeTime.Min = 1.0f;
    LifetimeModule->LifeTime.Max = 2.0f;

    UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
    SizeModule->StartSize.Operation = EDistributionMode::DOP_Constant;
    SizeModule->StartSize.Constant = FVector(1.0f, 1.0f, 1.0f);

    UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
    ColorModule->StartColor.Operation = EDistributionMode::DOP_Constant;
    ColorModule->StartColor.Constant = FVector(1.0f, 1.0f, 1.0f);

    // Modules 배열에 추가
    LODLevel->Modules.Add(LifetimeModule);
    LODLevel->Modules.Add(SizeModule);
    LODLevel->Modules.Add(ColorModule);

    // SpawnModules 배열에 추가 (초기화 시 실행되는 모듈들)
    LODLevel->SpawnModules.Add(LifetimeModule);
    LODLevel->SpawnModules.Add(SizeModule);
    LODLevel->SpawnModules.Add(ColorModule);

    // TypeDataModule은 Sprite의 경우 nullptr
    LODLevel->TypeDataModule = nullptr;

    // Emitter 정보 캐싱
    DefaultEmitter->CacheEmitterModuleInfo();

    // ParticleSystem에 기본 Emitter 추가
    State->CurrentParticleSystem->Emitters.Add(DefaultEmitter);

    // 첫 번째 Emitter 자동 선택
    State->SelectedEmitterIndex = 0;

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
