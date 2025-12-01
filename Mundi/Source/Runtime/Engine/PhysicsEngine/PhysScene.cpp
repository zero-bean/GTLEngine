#include "pch.h"
#include "PhysScene.h"

#include "BodyInstance.h"
#include "PrimitiveComponent.h"

// 커스텀 Simulation Filter Shader
// 랙돌 내 자체 충돌은 PxAggregate(selfCollision=false)로 처리
PxFilterFlags CustomFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // 트리거 처리
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    // 기본 충돌 처리
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    // CCD 활성화
    pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;

    return PxFilterFlag::eDEFAULT;
}

FPhysScene::FPhysScene(UWorld* InOwningWorld)
    : PhysXScene(nullptr)
    , OwningWorld(InOwningWorld)
    , bPhysXSceneExecuting(false)
{
}

FPhysScene::~FPhysScene()
{
    TermPhysScene();
}

void FPhysScene::StartFrame()
{
    if (OwningWorld)
    {
        float DeltaTime = OwningWorld->GetDeltaTime(EDeltaTime::Game);
        TickPhysScene(DeltaTime);
    }
}

void FPhysScene::EndFrame(ULineComponent* InLineComponent)
{
    WaitPhysScene();
    ProcessPhysScene();

    if (InLineComponent)
    {
        // @todo 디버그 라인 렌더링
    }
}

void FPhysScene::InitPhysScene()
{
    if (PhysXScene) { return; }

    if (!GPhysXSDK)
    {
        UE_LOG("[PhysX Error] PhysX SDK가 초기화되지 않았습니다.");
        return;
    }

    PxSceneDesc SceneDesc(GPhysXSDK->getTolerancesScale());

    if (!GPhysXDispatcher)
    {
        UE_LOG("[PhysX Error] PhysX Dispatcher가 초기화되지 않았습니다.");
    }
    SceneDesc.cpuDispatcher = GPhysXDispatcher;
    SceneDesc.filterShader = CustomFilterShader;

    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    SceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;

    SceneDesc.gravity = PxVec3(0, 0, -9.81);

    PhysXScene = GPhysXSDK->createScene(SceneDesc);

    if (PhysXScene)
    {
        if (PxPvdSceneClient* PVDClient = PhysXScene->getScenePvdClient())
        {
            PVDClient->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS |
                                        PxPvdSceneFlag::eTRANSMIT_CONTACTS    |
                                        PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES);
        }
        UE_LOG("[PhysX] PhysX Scene 생성에 성공했습니다.");
    }
    else
    {
        UE_LOG("[PhysX Error] PhysX Scene 생성에 실패했습니다.");
    }
}

void FPhysScene::TermPhysScene()
{
    if (PhysXScene)
    {
        WaitPhysScene();

        PhysXScene->release();
        PhysXScene = nullptr;
    }
}

void FPhysScene::TickPhysScene(float DeltaTime)
{
    if (!PhysXScene)          { return; }

    if (bPhysXSceneExecuting) { return; }

    if (DeltaTime <= 0.0f)    { return; }

    PhysXScene->simulate(DeltaTime);
    bPhysXSceneExecuting = true;
}

void FPhysScene::WaitPhysScene()
{
    if (!PhysXScene) { return; }

    if (bPhysXSceneExecuting)
    {
        PhysXScene->fetchResults(true);
        bPhysXSceneExecuting = false;
    }
}

void FPhysScene::ProcessPhysScene()
{
    if (!PhysXScene) { return; }

    SyncComponentsToBodies();
}

void FPhysScene::SyncComponentsToBodies()
{
    if (!PhysXScene) { return; }

    PxU32 NbActiveActors = 0;
    PxActor** ActiveActors = PhysXScene->getActiveActors(NbActiveActors);

    for (PxU32 i = 0; i < NbActiveActors; i++)
    {
        if (!ActiveActors[i]) { continue; }

        PxRigidActor* RigidActor = ActiveActors[i]->is<PxRigidActor>();

        if (!RigidActor) { continue; }

        // userData가 nullptr이면 이미 정리된 바디 (TermBody에서 클리어됨)
        void* UserData = RigidActor->userData;
        if (!UserData) { continue; }

        FBodyInstance* BodyInstance = static_cast<FBodyInstance*>(UserData);

        // BodyInstance 유효성 검사: RigidActor가 일치하는지 확인
        // (정리 중인 바디이거나 잘못된 포인터인 경우 방지)
        if (!BodyInstance->IsValidBodyInstance() || BodyInstance->RigidActor != RigidActor)
        {
            continue;
        }

        // 랙돌 바디는 SyncBonesFromPhysics()에서 별도 처리됨
        if (BodyInstance->bIsRagdollBody)
        {
            continue;
        }

        // OwnerComponent가 유효한지 확인
        UPrimitiveComponent* OwnerComp = BodyInstance->OwnerComponent;
        if (!OwnerComp)
        {
            continue;
        }

        FTransform NewTransform = BodyInstance->GetUnrealWorldTransform();
        NewTransform.Scale3D = OwnerComp->GetWorldScale();
        OwnerComp->SetWorldTransform(NewTransform, EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::None);
    }
}
