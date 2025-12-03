#pragma once

#include "ELinearConstraintMotion.h"
#include "EAngularConstraintMotion.h"
#include "FConstraintSetup.generated.h"

/**
 * FConstraintSetup
 *
 * 두 바디 간의 물리 제약 조건 설정
 * Unreal Engine 스타일의 Motion 기반 설계
 */
USTRUCT(DisplayName="제약 조건 설정", Description="두 바디 간의 물리 제약 조건 설정")
struct FConstraintSetup
{
	GENERATED_REFLECTION_BODY()

public:
	// ────────────────────────────────────────────────
	// 기본 정보
	// ────────────────────────────────────────────────
	UPROPERTY()
	FName JointName;

	// ────────────────────────────────────────────────
	// 연결된 바디
	// ────────────────────────────────────────────────
	UPROPERTY()
	int32 ParentBodyIndex = -1;

	UPROPERTY()
	int32 ChildBodyIndex = -1;

	// ────────────────────────────────────────────────
	// Linear Constraint (위치 제한)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="X축 선형 이동 타입")
	ELinearConstraintMotion LinearXMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="Y축 선형 이동 타입")
	ELinearConstraintMotion LinearYMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="Z축 선형 이동 타입")
	ELinearConstraintMotion LinearZMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="선형 이동 제한 거리 (cm)")
	float LinearLimit = 0.0f;

	// ────────────────────────────────────────────────
	// Angular Constraint - Swing (Cone 제한)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing1 (Y축 회전) 타입")
	EAngularConstraintMotion Swing1Motion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing2 (Z축 회전) 타입")
	EAngularConstraintMotion Swing2Motion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing1 제한 각도 (degrees)")
	float Swing1LimitDegrees = 45.0f;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing2 제한 각도 (degrees)")
	float Swing2LimitDegrees = 45.0f;

	// ────────────────────────────────────────────────
	// Angular Constraint - Twist (X축 회전)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Twist (X축 회전) 타입")
	EAngularConstraintMotion TwistMotion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Twist 제한 각도 (±degrees, 대칭)")
	float TwistLimitDegrees = 45.0f;

	// ────────────────────────────────────────────────
	// Drive (스프링/댐퍼)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Drive", Tooltip="스프링 강성")
	float Stiffness = 0.0f;

	UPROPERTY(EditAnywhere, Category="Drive", Tooltip="감쇠 계수")
	float Damping = 0.0f;

	// ────────────────────────────────────────────────
	// Constraint Frame - Child (Body1)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="자식 바디 로컬 연결점 위치")
	FVector ChildPosition = FVector::Zero();

	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="자식 프레임 회전 (Roll, Pitch, Yaw degrees)")
	FVector ChildRotation = FVector::Zero();

	// 축 벡터 (저장용 - UI 노출 안됨, Euler 변환 손실 방지)
	UPROPERTY()
	FVector ChildPriAxis = FVector(1, 0, 0);  // X축 (Twist)

	UPROPERTY()
	FVector ChildSecAxis = FVector(0, 1, 0);  // Y축

	// ────────────────────────────────────────────────
	// Constraint Frame - Parent (Body2)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="부모 바디 로컬 연결점 위치")
	FVector ParentPosition = FVector::Zero();

	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="부모 프레임 회전 (Roll, Pitch, Yaw degrees)")
	FVector ParentRotation = FVector::Zero();

	// 축 벡터 (저장용 - UI 노출 안됨, Euler 변환 손실 방지)
	UPROPERTY()
	FVector ParentPriAxis = FVector(1, 0, 0);  // X축 (Twist)

	UPROPERTY()
	FVector ParentSecAxis = FVector(0, 1, 0);  // Y축

	// ────────────────────────────────────────────────
	// Angular Rotation Offset (비대칭 Limit용)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="각도 제한 중심점 오프셋 (Roll, Pitch, Yaw degrees)")
	FVector AngularRotationOffset = FVector::Zero();

	// ────────────────────────────────────────────────
	// 에디터 상태 (직렬화 제외)
	// ────────────────────────────────────────────────
	bool bSelected = false;

	// ────────────────────────────────────────────────
	// 생성자
	// ────────────────────────────────────────────────
	FConstraintSetup() = default;

	FConstraintSetup(const FName& InJointName, int32 InParentIndex, int32 InChildIndex)
		: JointName(InJointName)
		, ParentBodyIndex(InParentIndex)
		, ChildBodyIndex(InChildIndex)
	{}

	// ────────────────────────────────────────────────
	// 헬퍼: Linear motion 중 하나라도 Limited인지
	// ────────────────────────────────────────────────
	bool HasLinearLimit() const
	{
		return LinearXMotion == ELinearConstraintMotion::Limited ||
		       LinearYMotion == ELinearConstraintMotion::Limited ||
		       LinearZMotion == ELinearConstraintMotion::Limited;
	}

	// ────────────────────────────────────────────────
	// 헬퍼: Angular motion 중 하나라도 Limited인지
	// ────────────────────────────────────────────────
	bool HasAngularLimit() const
	{
		return Swing1Motion == EAngularConstraintMotion::Limited ||
		       Swing2Motion == EAngularConstraintMotion::Limited ||
		       TwistMotion == EAngularConstraintMotion::Limited;
	}

	// ────────────────────────────────────────────────
	// 축 벡터 ↔ Euler 변환 헬퍼
	// ────────────────────────────────────────────────

	// Euler(degrees) → 축 벡터 변환 (UI에서 Euler 변경 시 호출)
	void UpdateParentAxesFromEuler()
	{
		FQuat Quat = FQuat::MakeFromEulerZYX(ParentRotation);
		ParentPriAxis = Quat.RotateVector(FVector(1, 0, 0));
		ParentSecAxis = Quat.RotateVector(FVector(0, 1, 0));
	}

	void UpdateChildAxesFromEuler()
	{
		FQuat Quat = FQuat::MakeFromEulerZYX(ChildRotation);
		ChildPriAxis = Quat.RotateVector(FVector(1, 0, 0));
		ChildSecAxis = Quat.RotateVector(FVector(0, 1, 0));
	}

	// 축 벡터 → Euler(degrees) 변환 (축 벡터 설정 후 UI 업데이트용)
	void UpdateParentEulerFromAxes()
	{
		FQuat Quat = GetQuatFromAxes(ParentPriAxis, ParentSecAxis);
		ParentRotation = Quat.ToEulerZYXDeg();
	}

	void UpdateChildEulerFromAxes()
	{
		FQuat Quat = GetQuatFromAxes(ChildPriAxis, ChildSecAxis);
		ChildRotation = Quat.ToEulerZYXDeg();
	}

	// 축 벡터에서 Quaternion 구성
	static FQuat GetQuatFromAxes(const FVector& PriAxis, const FVector& SecAxis)
	{
		// X = PriAxis, Y = SecAxis, Z = X × Y
		FVector X = PriAxis.GetNormalized();
		FVector Y = SecAxis.GetNormalized();
		FVector Z = FVector::Cross(X, Y).GetNormalized();
		// Y를 다시 직교화
		Y = FVector::Cross(Z, X).GetNormalized();

		// 회전 행렬에서 Quaternion 추출
		FMatrix RotMatrix;
		RotMatrix.M[0][0] = X.X; RotMatrix.M[0][1] = X.Y; RotMatrix.M[0][2] = X.Z; RotMatrix.M[0][3] = 0;
		RotMatrix.M[1][0] = Y.X; RotMatrix.M[1][1] = Y.Y; RotMatrix.M[1][2] = Y.Z; RotMatrix.M[1][3] = 0;
		RotMatrix.M[2][0] = Z.X; RotMatrix.M[2][1] = Z.Y; RotMatrix.M[2][2] = Z.Z; RotMatrix.M[2][3] = 0;
		RotMatrix.M[3][0] = 0;   RotMatrix.M[3][1] = 0;   RotMatrix.M[3][2] = 0;   RotMatrix.M[3][3] = 1;

		return FQuat(RotMatrix);
	}
};
