#include "pch.h"
#include "ControllerInstance.h"

#include <cmath>

#include "PhysScene.h"
#include "CapsuleComponent.h"
#include "CharacterMovementComponent.h"
#include "CCTQueryFilterCallback.h"
#include "CCTHitReport.h"
#include "CCTBehaviorCallback.h"

// PI 상수
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================================================================================
// FCCTSettings
// ==================================================================================

void FCCTSettings::SetDefaults()
{
    Radius = 0.5f;
    Height = 1.0f;
    StepOffset = 0.45f;
    SlopeLimit = 0.707f;  // cos(45도)
    ContactOffset = 0.1f;
    UpDirection = PxVec3(0, 0, 1);
    NonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
}

void FCCTSettings::SetFromComponents(UCapsuleComponent* Capsule, UCharacterMovementComponent* Movement)
{
    SetDefaults();

    if (Capsule)
    {
        // 프로젝트가 cm 단위를 직접 사용 (변환 없음)
        float RadiusCm = Capsule->GetCapsuleRadius();
        float HalfHeightCm = Capsule->GetCapsuleHalfHeight();

        Radius = RadiusCm;
        Height = (HalfHeightCm * 2.0f - RadiusCm * 2.0f);  // 원통 부분만
        if (Height < 0.0f) Height = 0.01f;  // 최소값 보장
    }

    if (Movement)
    {
        StepOffset = Movement->MaxStepHeight;  // cm 그대로
        // 각도를 라디안으로 변환 후 cos 계산
        float AngleRadians = Movement->WalkableFloorAngle * static_cast<float>(M_PI) / 180.0f;
        SlopeLimit = std::cos(AngleRadians);
    }
}

// ==================================================================================
// FControllerInstance
// ==================================================================================

FControllerInstance::FControllerInstance()
    : Controller(nullptr)
    , OwnerComponent(nullptr)
    , MovementComponent(nullptr)
    , PhysScene(nullptr)
    , CollisionChannel(ECollisionChannel::Pawn)
    , CollisionMask(CollisionMasks::DefaultPawn)
    , QueryFilter(nullptr)
    , bLastGrounded(false)
    , bLastTouchingCeiling(false)
    , bLastTouchingSide(false)
{
    CachedFilterData = PxFilterData();
}

FControllerInstance::~FControllerInstance()
{
    TermController();
}

void FControllerInstance::InitController(UCapsuleComponent* InCapsule, UCharacterMovementComponent* InMovement, FPhysScene* InPhysScene)
{
    if (!InPhysScene || !InPhysScene->GetControllerManager() || !InCapsule)
    {
        UE_LOG("[PhysX CCT] InitController 실패: 필수 파라미터가 nullptr");
        return;
    }

    OwnerComponent = InCapsule;
    MovementComponent = InMovement;
    PhysScene = InPhysScene;

    // 설정 추출
    FCCTSettings Settings;
    Settings.SetFromComponents(InCapsule, InMovement);

    // PxCapsuleControllerDesc 구성
    PxCapsuleControllerDesc Desc;
    Desc.radius = Settings.Radius;
    Desc.height = Settings.Height;
    Desc.stepOffset = Settings.StepOffset;
    Desc.slopeLimit = Settings.SlopeLimit;
    Desc.contactOffset = Settings.ContactOffset;
    Desc.upDirection = Settings.UpDirection;
    Desc.nonWalkableMode = Settings.NonWalkableMode;

    // 초기 위치 (프로젝트가 cm 단위 직접 사용)
    FVector WorldPos = InCapsule->GetWorldLocation();
    Desc.position = PxExtendedVec3(WorldPos.X, WorldPos.Y, WorldPos.Z);

    // Material (기본 마찰/반발 계수로 생성)
    PxMaterial* DefaultMaterial = GPhysXSDK->createMaterial(0.5f, 0.5f, 0.1f);
    Desc.material = DefaultMaterial;

    // 콜백 설정
    Desc.reportCallback = InPhysScene->GetCCTHitReport();
    Desc.behaviorCallback = InPhysScene->GetCCTBehaviorCallback();

    // 사용자 데이터
    Desc.userData = this;

    // 유효성 검사
    if (!Desc.isValid())
    {
        UE_LOG("[PhysX CCT] PxCapsuleControllerDesc 유효성 검사 실패");
        UE_LOG("[PhysX CCT] Radius: %.3f, Height: %.3f, StepOffset: %.3f", Desc.radius, Desc.height, Desc.stepOffset);
        return;
    }

    // Controller 생성
    Controller = InPhysScene->GetControllerManager()->createController(Desc);
    if (!Controller)
    {
        UE_LOG("[PhysX CCT] PxController 생성 실패");
        return;
    }

    // QueryFilter 생성
    QueryFilter = new FCCTQueryFilterCallback();
    QueryFilter->MyChannel = CollisionChannel;
    QueryFilter->MyCollisionMask = CollisionMask;

    // FilterData 캐시
    CachedFilterData.word0 = static_cast<PxU32>(CollisionChannel);
    CachedFilterData.word1 = CollisionMask;

    UE_LOG("[PhysX CCT] Controller 초기화 완료 (Radius: %.2f, Height: %.2f, StepOffset: %.2f)",
           Settings.Radius, Settings.Height, Settings.StepOffset);
}

void FControllerInstance::TermController()
{
    if (QueryFilter)
    {
        delete QueryFilter;
        QueryFilter = nullptr;
    }

    if (Controller)
    {
        Controller->release();
        Controller = nullptr;
    }

    OwnerComponent = nullptr;
    MovementComponent = nullptr;
    PhysScene = nullptr;
}

PxControllerCollisionFlags FControllerInstance::Move(const FVector& Displacement, float DeltaTime)
{
    if (!Controller)
    {
        return PxControllerCollisionFlags(0);
    }

    // 프로젝트가 cm 단위 직접 사용 (변환 없음)
    PxVec3 PxDisp = U2PVector(Displacement);

    // 필터 구성
    PxControllerFilters Filters = GetFilters();

    // 이동 수행
    PxControllerCollisionFlags CollisionFlags = Controller->move(PxDisp, 0.001f, DeltaTime, Filters);

    // 충돌 상태 캐시
    bLastGrounded = CollisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
    bLastTouchingCeiling = CollisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP);
    bLastTouchingSide = CollisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_SIDES);

    return CollisionFlags;
}

void FControllerInstance::SetPosition(const FVector& NewPosition)
{
    if (!Controller)
    {
        return;
    }

    // 프로젝트가 cm 단위 직접 사용 (변환 없음)
    PxExtendedVec3 PxPos(NewPosition.X, NewPosition.Y, NewPosition.Z);
    Controller->setPosition(PxPos);
}

FVector FControllerInstance::GetPosition() const
{
    if (!Controller)
    {
        return FVector::Zero();
    }

    PxExtendedVec3 PxPos = Controller->getPosition();
    // 프로젝트가 cm 단위 직접 사용 (변환 없음)
    return FVector(
        static_cast<float>(PxPos.x),
        static_cast<float>(PxPos.y),
        static_cast<float>(PxPos.z)
    );
}

FVector FControllerInstance::GetFootPosition() const
{
    if (!Controller)
    {
        return FVector::Zero();
    }

    PxExtendedVec3 PxPos = Controller->getFootPosition();
    // 프로젝트가 cm 단위 직접 사용 (변환 없음)
    return FVector(
        static_cast<float>(PxPos.x),
        static_cast<float>(PxPos.y),
        static_cast<float>(PxPos.z)
    );
}

PxControllerFilters FControllerInstance::GetFilters()
{
    PxControllerFilters Filters;
    Filters.mFilterData = &CachedFilterData;
    Filters.mFilterCallback = QueryFilter;
    Filters.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

    if (PhysScene)
    {
        Filters.mCCTFilterCallback = PhysScene->GetCCTControllerFilter();
    }

    return Filters;
}

void FControllerInstance::SetCollisionChannel(ECollisionChannel Channel, uint32 Mask)
{
    CollisionChannel = Channel;
    CollisionMask = Mask;

    CachedFilterData.word0 = static_cast<PxU32>(Channel);
    CachedFilterData.word1 = Mask;

    if (QueryFilter)
    {
        QueryFilter->MyChannel = Channel;
        QueryFilter->MyCollisionMask = Mask;
    }
}
