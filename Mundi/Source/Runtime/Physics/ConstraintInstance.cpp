#include "pch.h"
#include "ConstraintInstance.h"
#include "BodyInstance.h"
#include "PhysXConversion.h"
#include "PhysicsSystem.h"

using namespace physx;

void FConstraintInstance::InitConstraint(
    FBodyInstance* Body1,
    FBodyInstance* Body2,
    const FTransform& Frame1,
    const FTransform& Frame2)
{
    // 유효성 검사
    if (!Body1 || !Body2)
    {
        UE_LOG("[FConstraintInstance] InitConstraint failed: Body is null");
        return;
    }

    if (!Body1->RigidActor || !Body2->RigidActor)
    {
        UE_LOG("[FConstraintInstance] InitConstraint failed: RigidActor is null");
        return;
    }

    // 기존 Joint 해제
    TermConstraint();

    // PhysX Physics 획득
    FPhysicsSystem* PhysSystem = GEngine.GetPhysicsSystem();
    if (!PhysSystem)
    {
        UE_LOG("[FConstraintInstance] InitConstraint failed: PhysicsSystem is null");
        return;
    }

    PxPhysics* Physics = PhysSystem->GetPhysics();
    if (!Physics)
    {
        UE_LOG("[FConstraintInstance] InitConstraint failed: PxPhysics is null");
        return;
    }

    // 프레임 변환 (프로젝트 좌표계 → PhysX 좌표계)
    PxTransform PxFrame1 = PhysXConvert::ToPx(Frame1);
    PxTransform PxFrame2 = PhysXConvert::ToPx(Frame2);

    // D6Joint 생성
    PxD6Joint* D6Joint = PxD6JointCreate(
        *Physics,
        Body1->RigidActor, PxFrame1,
        Body2->RigidActor, PxFrame2
    );

    if (!D6Joint)
    {
        UE_LOG("[FConstraintInstance] InitConstraint failed: PxD6JointCreate returned null");
        return;
    }

    // 제한 설정 적용
    ConfigureLinearLimits(D6Joint);
    ConfigureAngularLimits(D6Joint);

    // Joint 저장
    PxJoint = D6Joint;

    //UE_LOG("[FConstraintInstance] Joint created: %s (%s <-> %s)",
    //    *JointName.ToString(), *ConstraintBone1.ToString(), *ConstraintBone2.ToString());
}

void FConstraintInstance::TermConstraint()
{
    if (PxJoint)
    {
        PxJoint->release();
        PxJoint = nullptr;
    }
}

void FConstraintInstance::ConfigureLinearLimits(PxD6Joint* Joint)
{
    if (!Joint) return;

    // 선형 모션 타입 변환 헬퍼
    auto ToPxMotion = [](ELinearConstraintMotion Motion) -> PxD6Motion::Enum
    {
        switch (Motion)
        {
            case ELinearConstraintMotion::Free:    return PxD6Motion::eFREE;
            case ELinearConstraintMotion::Limited: return PxD6Motion::eLIMITED;
            case ELinearConstraintMotion::Locked:
            default:                               return PxD6Motion::eLOCKED;
        }
    };

    // 각 축의 선형 모션 설정
    Joint->setMotion(PxD6Axis::eX, ToPxMotion(LinearXMotion));
    Joint->setMotion(PxD6Axis::eY, ToPxMotion(LinearYMotion));
    Joint->setMotion(PxD6Axis::eZ, ToPxMotion(LinearZMotion));

    // 선형 제한 설정 (Limited인 경우에만 의미 있음)
    if (LinearLimit > 0.0f)
    {
        bool bHasLimited = (LinearXMotion == ELinearConstraintMotion::Limited) ||
                          (LinearYMotion == ELinearConstraintMotion::Limited) ||
                          (LinearZMotion == ELinearConstraintMotion::Limited);

        if (bHasLimited)
        {
            FPhysicsSystem* PhysSystem = GEngine.GetPhysicsSystem();
            if (PhysSystem && PhysSystem->GetPhysics())
            {
                PxTolerancesScale Scale = PhysSystem->GetPhysics()->getTolerancesScale();
                PxJointLinearLimit LinearLimitPx(Scale, LinearLimit);
                Joint->setLinearLimit(LinearLimitPx);
            }
        }
    }
}

void FConstraintInstance::ConfigureAngularLimits(PxD6Joint* Joint)
{
    if (!Joint) return;

    // 각도 모션 타입 변환 헬퍼
    auto ToPxMotion = [](EAngularConstraintMotion Motion) -> PxD6Motion::Enum
    {
        switch (Motion)
        {
            case EAngularConstraintMotion::Free:    return PxD6Motion::eFREE;
            case EAngularConstraintMotion::Limited: return PxD6Motion::eLIMITED;
            case EAngularConstraintMotion::Locked:
            default:                                return PxD6Motion::eLOCKED;
        }
    };

    // 각 축의 각도 모션 설정
    Joint->setMotion(PxD6Axis::eTWIST,  ToPxMotion(AngularTwistMotion));
    Joint->setMotion(PxD6Axis::eSWING1, ToPxMotion(AngularSwing1Motion));
    Joint->setMotion(PxD6Axis::eSWING2, ToPxMotion(AngularSwing2Motion));

    // 각도 제한값을 라디안으로 변환
    float TwistRad  = PhysXConvert::DegreesToRadians(TwistLimitAngle);
    float Swing1Rad = PhysXConvert::DegreesToRadians(Swing1LimitAngle);
    float Swing2Rad = PhysXConvert::DegreesToRadians(Swing2LimitAngle);

    // Twist 제한 설정
    if (AngularTwistMotion == EAngularConstraintMotion::Limited)
    {
        PxJointAngularLimitPair TwistLimit(-TwistRad, TwistRad);
        Joint->setTwistLimit(TwistLimit);
    }

    // Swing 제한 설정 (Cone limit)
    if (AngularSwing1Motion == EAngularConstraintMotion::Limited ||
        AngularSwing2Motion == EAngularConstraintMotion::Limited)
    {
        // Swing1, Swing2 중 하나라도 Limited면 Cone limit 설정
        float UsedSwing1 = (AngularSwing1Motion == EAngularConstraintMotion::Limited) ? Swing1Rad : PxPi;
        float UsedSwing2 = (AngularSwing2Motion == EAngularConstraintMotion::Limited) ? Swing2Rad : PxPi;

        PxJointLimitCone SwingLimit(UsedSwing1, UsedSwing2);
        Joint->setSwingLimit(SwingLimit);
    }
}
