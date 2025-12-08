#include "pch.h"
#include "BoneSocketComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMesh.h"
#include "BodyInstance.h"
#include "EPhysicsMode.h"

#include <PxRigidDynamic.h>
using namespace physx;

UBoneSocketComponent::UBoneSocketComponent()
{
    // 기본 오프셋 초기화 (단위 변환)
    SocketOffset = FTransform();
}

void UBoneSocketComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG("[BoneSocketComponent] ===== BeginPlay START =====");
    UE_LOG("[BoneSocketComponent] Component: %s, BoneName='%s', BoneIndex=%d", GetName().c_str(), BoneName.c_str(), BoneIndex);

    // TargetMesh가 설정되지 않았으면 부모에서 자동으로 찾기
    if (!TargetMesh)
    {
        FindAndSetTargetFromParent();
    }
    UE_LOG("[BoneSocketComponent] TargetMesh=%p", TargetMesh);

    // BoneName이 설정되어 있고 BoneIndex가 -1이면 이름으로 인덱스 찾기
    if (TargetMesh && BoneIndex == -1 && !BoneName.empty())
    {
        USkeletalMesh* SkelMesh = TargetMesh->GetSkeletalMesh();
        UE_LOG("[BoneSocketComponent] SkelMesh=%p", SkelMesh);

        if (SkelMesh)
        {
            const FSkeleton* Skeleton = SkelMesh->GetSkeleton();
            UE_LOG("[BoneSocketComponent] Skeleton=%p", Skeleton);

            if (Skeleton)
            {
                // 모든 본 이름 출력 (디버깅용)
                UE_LOG("[BoneSocketComponent] Available bones in target mesh:");
                for (const auto& Pair : Skeleton->BoneNameToIndex)
                {
                    if (Pair.first.find("Hand") != FString::npos ||
                        Pair.first.find("hand") != FString::npos ||
                        Pair.first.find("Arm") != FString::npos)
                    {
                        UE_LOG("  - %s (index: %d)", Pair.first.c_str(), Pair.second);
                    }
                }

                // 먼저 정확한 이름으로 검색
                auto It = Skeleton->BoneNameToIndex.find(BoneName);
                if (It != Skeleton->BoneNameToIndex.end())
                {
                    BoneIndex = It->second;
                    UE_LOG("[BoneSocketComponent] Found bone '%s' at index %d", BoneName.c_str(), BoneIndex);
                }
                else
                {
                    // 접두사 변형 시도 (mixamorig:, mixamorig4:, 접두사 없음)
                    FString BaseName = BoneName;

                    // 기존 접두사 제거
                    if (BaseName.find("mixamorig4:") == 0)
                    {
                        BaseName = BaseName.substr(11);
                    }
                    else if (BaseName.find("mixamorig:") == 0)
                    {
                        BaseName = BaseName.substr(10);
                    }

                    UE_LOG("[BoneSocketComponent] Trying variants for BaseName='%s'", BaseName.c_str());

                    // 다양한 접두사로 시도
                    const char* Prefixes[] = { "mixamorig:", "mixamorig4:", "mixamorig9:", "" };
                    for (const char* Prefix : Prefixes)
                    {
                        FString TestName = FString(Prefix) + BaseName;
                        UE_LOG("[BoneSocketComponent] Trying: '%s'", TestName.c_str());
                        auto TestIt = Skeleton->BoneNameToIndex.find(TestName);
                        if (TestIt != Skeleton->BoneNameToIndex.end())
                        {
                            BoneIndex = TestIt->second;
                            BoneName = TestName;  // 찾은 이름으로 업데이트
                            UE_LOG("[BoneSocketComponent] SUCCESS! Found bone '%s' at index %d", TestName.c_str(), BoneIndex);
                            break;
                        }
                    }

                    // 여전히 못 찾았으면 로그 출력
                    if (BoneIndex == -1)
                    {
                        UE_LOG("[BoneSocketComponent] FAILED: Bone '%s' not found!", BoneName.c_str());
                    }
                }
            }
        }
    }
    else
    {
        UE_LOG("[BoneSocketComponent] Skipping bone search: TargetMesh=%p, BoneIndex=%d, BoneName.empty()=%d",
               TargetMesh, BoneIndex, BoneName.empty() ? 1 : 0);
    }

    UE_LOG("[BoneSocketComponent] ===== BeginPlay END: BoneIndex=%d =====", BoneIndex);

    // 오프셋 프로퍼티에서 SocketOffset 계산
    UpdateSocketOffsetFromProperties();

    // 초기 동기화
    SyncWithBone();
}

void UBoneSocketComponent::SetTarget(USkeletalMeshComponent* InTargetMesh, int32 InBoneIndex)
{
    TargetMesh = InTargetMesh;
    BoneIndex = InBoneIndex;

    // 본 이름도 동기화
    if (TargetMesh && BoneIndex >= 0)
    {
        if (USkeletalMesh* SkelMesh = TargetMesh->GetSkeletalMesh())
        {
            if (const FSkeleton* Skeleton = SkelMesh->GetSkeleton())
            {
                if (BoneIndex < static_cast<int32>(Skeleton->Bones.Num()))
                {
                    BoneName = Skeleton->Bones[BoneIndex].Name;
                }
            }
        }
    }

    // 즉시 동기화
    SyncWithBone();
}

bool UBoneSocketComponent::SetTargetByName(USkeletalMeshComponent* InTargetMesh, const FString& InBoneName)
{
    if (!InTargetMesh)
    {
        return false;
    }

    USkeletalMesh* SkelMesh = InTargetMesh->GetSkeletalMesh();
    if (!SkelMesh)
    {
        UE_LOG("[BoneSocketComponent] SetTargetByName: No SkeletalMesh");
        return false;
    }

    const FSkeleton* Skeleton = SkelMesh->GetSkeleton();
    if (!Skeleton)
    {
        UE_LOG("[BoneSocketComponent] SetTargetByName: No Skeleton");
        return false;
    }

    // 먼저 정확한 이름으로 검색
    auto It = Skeleton->BoneNameToIndex.find(InBoneName);
    if (It != Skeleton->BoneNameToIndex.end())
    {
        TargetMesh = InTargetMesh;
        BoneIndex = It->second;
        BoneName = InBoneName;
        UE_LOG("[BoneSocketComponent] SetTargetByName: Found '%s' at index %d", BoneName.c_str(), BoneIndex);
        SyncWithBone();
        return true;
    }

    // 접두사 변형 시도
    FString BaseName = InBoneName;
    if (BaseName.find("mixamorig4:") == 0)
    {
        BaseName = BaseName.substr(11);
    }
    else if (BaseName.find("mixamorig:") == 0)
    {
        BaseName = BaseName.substr(10);
    }

    const char* Prefixes[] = { "mixamorig:", "mixamorig4:", "mixamorig9:", "" };
    for (const char* Prefix : Prefixes)
    {
        FString TestName = FString(Prefix) + BaseName;
        auto TestIt = Skeleton->BoneNameToIndex.find(TestName);
        if (TestIt != Skeleton->BoneNameToIndex.end())
        {
            TargetMesh = InTargetMesh;
            BoneIndex = TestIt->second;
            BoneName = TestName;
            UE_LOG("[BoneSocketComponent] SetTargetByName: Found variant '%s' at index %d", BoneName.c_str(), BoneIndex);
            SyncWithBone();
            return true;
        }
    }

    // 못 찾았으면 모든 본 출력
    UE_LOG("[BoneSocketComponent] SetTargetByName: Bone '%s' not found!", InBoneName.c_str());
    UE_LOG("[BoneSocketComponent] BoneNameToIndex.size() = %d", (int)Skeleton->BoneNameToIndex.size());
    UE_LOG("[BoneSocketComponent] Bones.Num() = %d", (int)Skeleton->Bones.Num());

    // 처음 15개 본 이름 출력
    int32 Count = FMath::Min(15, static_cast<int32>(Skeleton->Bones.Num()));
    for (int32 i = 0; i < Count; ++i)
    {
        const FString& Name = Skeleton->Bones[i].Name;
        UE_LOG("[BoneSocketComponent] Bone[%d] = '%s' (len=%d)", i, Name.c_str(), (int)Name.length());
    }

    // 맵에서도 출력 시도 (처음 15개)
    UE_LOG("[BoneSocketComponent] From BoneNameToIndex map:");
    int32 MapCount = 0;
    for (const auto& Pair : Skeleton->BoneNameToIndex)
    {
        if (MapCount++ >= 15) break;
        UE_LOG("[BoneSocketComponent] Map: '%s' -> %d", Pair.first.c_str(), Pair.second);
    }
    return false;
}

USkeletalMeshComponent* UBoneSocketComponent::FindAndSetTargetFromParent()
{
    // 부모 컴포넌트 체인을 탐색하며 SkeletalMeshComponent 찾기
    USceneComponent* Parent = GetAttachParent();
    while (Parent)
    {
        // 현재 부모가 SkeletalMeshComponent인지 확인
        USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(Parent);
        if (SkelMeshComp)
        {
            TargetMesh = SkelMeshComp;
            return SkelMeshComp;
        }

        // 다음 부모로 이동
        Parent = Parent->GetAttachParent();
    }

    return nullptr;
}

bool UBoneSocketComponent::SetBoneIndexAuto(int32 InBoneIndex)
{
    // 부모에서 SkeletalMeshComponent 찾기
    USkeletalMeshComponent* FoundMesh = FindAndSetTargetFromParent();
    if (!FoundMesh)
    {
        return false;
    }

    SetTarget(FoundMesh, InBoneIndex);
    return true;
}

bool UBoneSocketComponent::SetBoneNameAuto(const FString& InBoneName)
{
    // 부모에서 SkeletalMeshComponent 찾기
    USkeletalMeshComponent* FoundMesh = FindAndSetTargetFromParent();
    if (!FoundMesh)
    {
        return false;
    }

    return SetTargetByName(FoundMesh, InBoneName);
}

void UBoneSocketComponent::SetSocketOffset(const FTransform& InOffset)
{
    SocketOffset = InOffset;

    // UPROPERTY들도 동기화
    SocketOffsetLocation = InOffset.Translation;
    SocketOffsetRotationEuler = InOffset.Rotation.ToEulerZYXDeg();
    SocketOffsetScale = InOffset.Scale3D;
}

void UBoneSocketComponent::SetSocketOffsetLocation(const FVector& InLocation)
{
    SocketOffsetLocation = InLocation;
    UpdateSocketOffsetFromProperties();
}

void UBoneSocketComponent::SetSocketOffsetRotation(const FQuat& InRotation)
{
    SocketOffsetRotationEuler = InRotation.ToEulerZYXDeg();
    UpdateSocketOffsetFromProperties();
}

void UBoneSocketComponent::UpdateSocketOffsetFromProperties()
{
    FQuat Rotation = FQuat::MakeFromEulerZYX(SocketOffsetRotationEuler);
    SocketOffset = FTransform(SocketOffsetLocation, Rotation, SocketOffsetScale);
}

void UBoneSocketComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (bAutoSync)
    {
        SyncWithBone();
    }
}

void UBoneSocketComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
    // 부모 트랜스폼이 변경되면 즉시 본과 재동기화
    // SyncWithBone 내부에서 bSyncing 플래그로 재귀 방지
    if (bAutoSync && TargetMesh)
    {
        SyncWithBone();
    }

    // 자식들에게 트랜스폼 변경 전파 (기본 클래스 구현 사용)
    Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

void UBoneSocketComponent::SyncWithBone()
{
    if (bSyncing)
    {
        return;
    }

    bSyncing = true;

    // BoneIndex가 유효하지 않아도 AttachedRagdoll이 있으면 동기화 수행
    if (!TargetMesh || BoneIndex < 0)
    {
        // AttachedRagdoll만 동기화
        SyncAttachedRagdoll();
        bSyncing = false;
        return;
    }

    FTransform BoneWorldTransform;

    // 래그돌 모드일 때는 직접 물리 바디에서 트랜스폼을 가져옴 (틱 순서 문제 해결)
    if (TargetMesh->GetPhysicsMode() == EPhysicsMode::Ragdoll)
    {
        FBodyInstance* Body = TargetMesh->GetBodyInstance(BoneIndex);
        if (Body && Body->IsValidBodyInstance())
        {
            BoneWorldTransform = Body->GetUnrealWorldTransform();
            // 스케일은 컴포넌트에서 가져오기 (물리 바디는 스케일 정보가 없을 수 있음)
            BoneWorldTransform.Scale3D = TargetMesh->GetWorldScale();
        }
        else
        {
            // 바디가 없는 본은 기존 방식 사용
            BoneWorldTransform = TargetMesh->GetBoneWorldTransform(BoneIndex);
        }
    }
    else
    {
        // 애니메이션/키네마틱 모드: 기존 방식
        BoneWorldTransform = TargetMesh->GetBoneWorldTransform(BoneIndex);
    }

    // 오프셋 적용: BoneWorld * SocketOffset
    FTransform FinalTransform = BoneWorldTransform.GetWorldTransform(SocketOffset);

    // 이 컴포넌트의 월드 트랜스폼 설정
    SetWorldTransform(FinalTransform, EUpdateTransformFlags::SkipPhysicsUpdate);

    // 부착된 래그돌 또는 자식 래그돌 동기화
    SyncAttachedRagdoll();

    bSyncing = false;
}

FTransform UBoneSocketComponent::GetSocketWorldTransform() const
{
    if (!TargetMesh || BoneIndex < 0)
    {
        return GetWorldTransform();
    }

    FTransform BoneWorldTransform;

    // 래그돌 모드일 때는 직접 물리 바디에서 트랜스폼을 가져옴
    if (TargetMesh->GetPhysicsMode() == EPhysicsMode::Ragdoll)
    {
        FBodyInstance* Body = TargetMesh->GetBodyInstance(BoneIndex);
        if (Body && Body->IsValidBodyInstance())
        {
            BoneWorldTransform = Body->GetUnrealWorldTransform();
            BoneWorldTransform.Scale3D = TargetMesh->GetWorldScale();
        }
        else
        {
            BoneWorldTransform = const_cast<USkeletalMeshComponent*>(TargetMesh)->GetBoneWorldTransform(BoneIndex);
        }
    }
    else
    {
        BoneWorldTransform = const_cast<USkeletalMeshComponent*>(TargetMesh)->GetBoneWorldTransform(BoneIndex);
    }

    return BoneWorldTransform.GetWorldTransform(SocketOffset);
}

// ──────────────────────────────
// 래그돌 부착 (시체 메기 등)
// ──────────────────────────────

void UBoneSocketComponent::AttachRagdoll(USkeletalMeshComponent* RagdollMesh, int32 AttachBoneIndex)
{
    if (!RagdollMesh)
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: RagdollMesh is null");
        return;
    }

    // 이전 래그돌이 있으면 해제
    if (AttachedRagdoll)
    {
        DetachRagdoll();
    }

    AttachedRagdoll = RagdollMesh;
    AttachedRagdollBoneIndex = AttachBoneIndex;

    // PhysicsAsset 확인
    UPhysicsAsset* PhysAsset = RagdollMesh->GetPhysicsAsset();
    if (!PhysAsset)
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: WARNING - No PhysicsAsset! Ragdoll won't work properly.");
    }
    else
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: PhysicsAsset found");
    }

    // 래그돌 모드가 아니면 래그돌 모드로 전환
    if (RagdollMesh->GetPhysicsMode() != EPhysicsMode::Ragdoll)
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: Setting Ragdoll mode");
        RagdollMesh->SetPhysicsMode(EPhysicsMode::Ragdoll);
    }

    // Bodies 확인
    const TArray<FBodyInstance*>& Bodies = RagdollMesh->GetBodies();
    UE_LOG("[BoneSocketComponent] AttachRagdoll: Bodies count = %d", Bodies.Num());

    // 고정할 본의 바디를 키네마틱으로 설정
    FBodyInstance* AttachBody = RagdollMesh->GetBodyInstance(AttachBoneIndex);
    if (AttachBody && AttachBody->IsValidBodyInstance())
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: Setting bone %d to kinematic", AttachBoneIndex);
        AttachBody->SetKinematic(true); // 키네마틱으로 전환
    }
    else
    {
        UE_LOG("[BoneSocketComponent] AttachRagdoll: WARNING - No body for bone index %d", AttachBoneIndex);
    }

    // 즉시 동기화
    SyncAttachedRagdoll();
}

void UBoneSocketComponent::DetachRagdoll()
{
    if (!AttachedRagdoll)
    {
        return;
    }

    // 고정했던 본의 바디를 다시 시뮬레이션 모드로
    FBodyInstance* AttachBody = AttachedRagdoll->GetBodyInstance(AttachedRagdollBoneIndex);
    if (AttachBody && AttachBody->IsValidBodyInstance())
    {
        AttachBody->SetKinematic(false); // 다시 시뮬레이션 (다이나믹)
    }

    AttachedRagdoll = nullptr;
    AttachedRagdollBoneIndex = -1;
}

void UBoneSocketComponent::SyncAttachedRagdoll()
{
    // 오프셋 프로퍼티 업데이트
    UpdateSocketOffsetFromProperties();

    // 소켓의 월드 위치 계산
    // TargetMesh와 BoneIndex가 유효하면 본 트랜스폼 사용, 아니면 부모 트랜스폼 사용
    FTransform BaseWorld;
    if (TargetMesh && BoneIndex >= 0)
    {
        // 래그돌 모드일 때는 물리 바디에서 트랜스폼 가져옴
        if (TargetMesh->GetPhysicsMode() == EPhysicsMode::Ragdoll)
        {
            FBodyInstance* Body = TargetMesh->GetBodyInstance(BoneIndex);
            if (Body && Body->IsValidBodyInstance())
            {
                BaseWorld = Body->GetUnrealWorldTransform();
                BaseWorld.Scale3D = TargetMesh->GetWorldScale();
            }
            else
            {
                BaseWorld = TargetMesh->GetBoneWorldTransform(BoneIndex);
            }
        }
        else
        {
            BaseWorld = TargetMesh->GetBoneWorldTransform(BoneIndex);
        }
    }
    else
    {
        // 본 추적이 안되면 부모 트랜스폼 사용
        BaseWorld = GetAttachParent() ? GetAttachParent()->GetWorldTransform() : FTransform();
    }
    FTransform SocketWorld = BaseWorld.GetWorldTransform(SocketOffset);

    // 명시적으로 AttachRagdoll로 부착된 경우
    if (AttachedRagdoll && AttachedRagdollBoneIndex >= 0)
    {
        FBodyInstance* AttachBody = AttachedRagdoll->GetBodyInstance(AttachedRagdollBoneIndex);
        if (AttachBody && AttachBody->IsValidBodyInstance())
        {
            // 매 프레임 키네마틱으로 강제 설정 후 위치 적용
            AttachBody->SetKinematic(true);
            AttachBody->SetKinematicTarget(SocketWorld); // 부드러운 이동
        }
        return;
    }

    // 자식 컴포넌트 중 래그돌 SkeletalMeshComponent 자동 감지 및 동기화
    for (USceneComponent* Child : GetAttachChildren())
    {
        USkeletalMeshComponent* SkelMeshChild = Cast<USkeletalMeshComponent>(Child);
        if (!SkelMeshChild)
        {
            continue;
        }

        // Ragdoll 또는 Kinematic 모드에서만 동기화
        EPhysicsMode ChildMode = SkelMeshChild->GetPhysicsMode();
        if (ChildMode != EPhysicsMode::Ragdoll && ChildMode != EPhysicsMode::Kinematic)
        {
            continue;
        }

        // 첫 번째 유효한 바디를 찾아서 루트로 사용
        const TArray<FBodyInstance*>& Bodies = SkelMeshChild->GetBodies();
        FBodyInstance* RootBody = nullptr;

        for (FBodyInstance* Body : Bodies)
        {
            if (Body && Body->IsValidBodyInstance())
            {
                RootBody = Body;
                break;
            }
        }

        if (RootBody)
        {
            // 매 프레임 키네마틱으로 강제 설정 후 위치 적용
            RootBody->SetKinematic(true);
            RootBody->SetKinematicTarget(SocketWorld); // 부드러운 이동

            // 다른 바디들에 높은 damping 적용하여 회전 억제
            for (FBodyInstance* Body : Bodies)
            {
                if (Body && Body != RootBody && Body->IsValidBodyInstance() && Body->RigidActor)
                {
                    PxRigidDynamic* DynActor = Body->RigidActor->is<PxRigidDynamic>();
                    if (DynActor)
                    {
                        DynActor->setLinearDamping(5.0f);
                        DynActor->setAngularDamping(5.0f);
                    }
                }
            }
        }
    }
}
