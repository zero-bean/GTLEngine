#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Editor/Public/GizmoTypes.h"
#include "Core/Public/Object.h"
#include "Actor/Public/Actor.h"

class UCamera;
class UConfigManager;

/**
 * @brief Gizmo 렌더링용 배칭 구조체
 * - 여러 메쉬를 한 번에 수집하고 일괄 렌더링
 * - 버퍼 생성/해제 보일러플레이트 제거
 */
struct FGizmoBatchRenderer
{
	struct FBatchedMesh
	{
		TArray<FNormalVertex> Vertices;
		TArray<uint32> Indices;
		FVector4 Color;
		FVector Location;
		FQuaternion Rotation;
		FVector Scale;
		bool bAlwaysVisible;

		FBatchedMesh()
			: Color(1, 1, 1, 1)
			, Location(0, 0, 0)
			, Rotation(FQuaternion::Identity())
			, Scale(1, 1, 1)
			, bAlwaysVisible(true)
		{}
	};

	TArray<FBatchedMesh> Meshes;

	void AddMesh(const TArray<FNormalVertex>& InVertices, const TArray<uint32>& InIndices,
	             const FVector4& InColor, const FVector& InLocation,
	             const FQuaternion& InRotation = FQuaternion::Identity(),
	             const FVector& InScale = FVector(1, 1, 1));

	void FlushAndRender(const FRenderState& InRenderState);

	void Clear();
};

class UGizmo :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UGizmo, UObject)
	
public:
	UGizmo();
	~UGizmo() override;
	void UpdateScale(const UCamera* InCamera, const D3D11_VIEWPORT& InViewport);
	void RenderGizmo(UCamera* InCamera, const D3D11_VIEWPORT& InViewport);
	void RenderForHitProxy(UCamera* InCamera, const D3D11_VIEWPORT& InViewport);
	void CollectRotationAngleOverlay(class FD2DOverlayManager& OverlayManager, UCamera* InCamera, const D3D11_VIEWPORT& InViewport, bool bUseViewportManager = true, bool bCustomSnapEnabled = false, float CustomSnapAngleDegrees = 0.0f);
	void ChangeGizmoMode();
	void SetGizmoMode(EGizmoMode Mode);

	/**
	 * @brief 기즈모 드래그 처리 함수들 (Editor와 Viewer에서 공통으로 사용)
	 * @param InCamera 현재 카메라
	 * @param WorldRay 마우스 레이
	 * @param InObjectPicker 오브젝트 피커 (레이-평면 충돌 계산용)
	 * @param ViewportRect 뷰포트 영역 (Rotation 계산용)
	 * @param bUseCustomSnap Rotation 스냅 설정 사용 여부 (true면 커스텀, false면 ViewportManager)
	 * @return 새로운 Location/Rotation/Scale 값
	 */
	FVector ProcessDragLocation(UCamera* InCamera, FRay& WorldRay, class UObjectPicker* InObjectPicker, bool bUseCustomSnap = false);
	FQuaternion ProcessDragRotation(UCamera* InCamera, FRay& WorldRay, const FRect& ViewportRect, bool bUseCustomSnap = false);
	FVector ProcessDragScale(UCamera* InCamera, FRay& WorldRay, class UObjectPicker* InObjectPicker);

	/**
	 * @brief Setter
	 */
	void SetLocation(const FVector& Location) const;
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }
	void SetComponentRotation(const FQuaternion& Rotation) const { TargetComponent->SetWorldRotation(Rotation); }
	void SetComponentScale(const FVector& Scale) const { TargetComponent->SetWorldScale3D(Scale); }
	void SetPreviousMouseLocation(const FVector& Location) { PreviousMouseLocation = Location; }
	void SetCurrentRotationAngle(float Angle) { CurrentRotationAngle = Angle; }
	void SetPreviousScreenPos(const FVector2& ScreenPos) { PreviousScreenPos = ScreenPos; }
	void SetDragStartScreenPos(const FVector2& ScreenPos) { DragStartScreenPos = ScreenPos; }
	void SetDragStartActorLocation(const FVector& Location) { DragStartActorLocation = Location; }
	void SetDragStartActorRotation(const FVector& Rotation) { DragStartActorRotation = Rotation; }
	void SetDragStartActorRotationQuat(const FQuaternion& Rotation) { DragStartActorRotationQuat = Rotation; }
	void SetDragStartActorScale(const FVector& Scale) { DragStartActorScale = Scale; }
	void SetDragStartMouseLocation(const FVector& Location) { DragStartMouseLocation = Location; }

	void SetWorld() { bIsWorld = true; }
	void SetLocal() { bIsWorld = false; }
	bool IsWorldMode() const { return bIsWorld; }

	// Location Snap 설정 (뷰어용 독립 설정)
	void SetUseCustomLocationSnap(bool bEnable) { bUseCustomLocationSnap = bEnable; }
	void SetCustomLocationSnapEnabled(bool bEnable) { bCustomLocationSnapEnabled = bEnable; }
	void SetCustomLocationSnapValue(float Value) { CustomLocationSnapValue = Value; }
	bool ShouldUseCustomLocationSnap() const { return bUseCustomLocationSnap; }
	bool IsCustomLocationSnapEnabled() const { return bCustomLocationSnapEnabled; }
	float GetCustomLocationSnapValue() const { return CustomLocationSnapValue; }

	// Rotation Snap 설정 (뷰어용 독립 설정)
	void SetUseCustomRotationSnap(bool bEnable) { bUseCustomRotationSnap = bEnable; }
	void SetCustomRotationSnapEnabled(bool bEnable) { bCustomRotationSnapEnabled = bEnable; }
	void SetCustomRotationSnapAngle(float Angle) { CustomRotationSnapAngle = Angle; }
	bool ShouldUseCustomRotationSnap() const { return bUseCustomRotationSnap; }
	bool IsCustomRotationSnapEnabled() const { return bCustomRotationSnapEnabled; }
	float GetCustomRotationSnapAngle() const { return CustomRotationSnapAngle; }

	// Scale Snap 설정 (뷰어용 독립 설정)
	void SetUseCustomScaleSnap(bool bEnable) { bUseCustomScaleSnap = bEnable; }
	void SetCustomScaleSnapEnabled(bool bEnable) { bCustomScaleSnapEnabled = bEnable; }
	void SetCustomScaleSnapValue(float Value) { CustomScaleSnapValue = Value; }
	bool ShouldUseCustomScaleSnap() const { return bUseCustomScaleSnap; }
	bool IsCustomScaleSnapEnabled() const { return bCustomScaleSnapEnabled; }
	float GetCustomScaleSnapValue() const { return CustomScaleSnapValue; }

	// Pilot Mode 기즈모 고정 위치 & 본 뷰어 기즈모용
	void SetFixedLocation(const FVector& InFixedLocation) { bUseFixedLocation = true; FixedLocation = InFixedLocation; }
	void ClearFixedLocation() { bUseFixedLocation = false; }
	bool IsFixedLocation() const { return bUseFixedLocation; }
	FVector GetFixedLocation() const { return FixedLocation; }

	// Getter
	float GetTranslateScale() const { return TranslateCollisionConfig.Scale; }
	float GetRotateScale() const { return RotateCollisionConfig.Scale; }
	EGizmoDirection GetGizmoDirection() const { return GizmoDirection; }
	FVector GetGizmoLocation()
	{
		// Pilot Mode일 때 고정 위치 사용
		if (bUseFixedLocation)
		{
			return FixedLocation;
		}
		return Primitives[static_cast<int>(GizmoMode)].Location;
	}
	FQuaternion GetComponentRotation() const { return TargetComponent->GetWorldRotationAsQuaternion(); }
	FVector GetComponentScale() const { return TargetComponent->GetWorldScale3D(); }
	FVector GetDragStartMouseLocation() { return DragStartMouseLocation; }
	FVector GetDragStartActorLocation() { return DragStartActorLocation; }
	FVector GetDragStartActorRotation() { return DragStartActorRotation; }
	FQuaternion GetDragStartActorRotationQuat() const { return DragStartActorRotationQuat; }
	FVector GetDragStartActorScale() { return DragStartActorScale; }
	EGizmoMode GetGizmoMode() const { return GizmoMode; }
	FVector GetGizmoAxis() const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::Forward:	return {1, 0, 0};  // X축 회전 (YZ 평면)
		case EGizmoDirection::Right:	return {0, 1, 0};  // Y축 회전 (XZ 평면)
		case EGizmoDirection::Up:		return {0, 0, 1};  // Z축 회전 (XY 평면)
		default:						return {0, 1, 0};
		}
	}

	// 평면 법선 반환 (평면 방향에 수직인 축)
	FVector GetPlaneNormal() const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::XY_Plane:	return {0, 0, 1};  // Z축
		case EGizmoDirection::XZ_Plane:	return {0, 1, 0};  // Y축
		case EGizmoDirection::YZ_Plane:	return {1, 0, 0};  // X축
		default:						return {0, 0, 1};
		}
	}

	// 평면의 두 접선 벡터 반환
	void GetPlaneTangents(FVector& OutTangent1, FVector& OutTangent2) const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::XY_Plane:
			OutTangent1 = {1, 0, 0};  // X축
			OutTangent2 = {0, 1, 0};  // Y축
			break;
		case EGizmoDirection::XZ_Plane:
			OutTangent1 = {1, 0, 0};  // X축
			OutTangent2 = {0, 0, 1};  // Z축
			break;
		case EGizmoDirection::YZ_Plane:
			OutTangent1 = {0, 1, 0};  // Y축
			OutTangent2 = {0, 0, 1};  // Z축
			break;
		default:
			OutTangent1 = {1, 0, 0};
			OutTangent2 = {0, 1, 0};
			break;
		}
	}

	// 방향이 평면인지 확인
	bool IsPlaneDirection() const
	{
		return GizmoDirection == EGizmoDirection::XY_Plane ||
		       GizmoDirection == EGizmoDirection::XZ_Plane ||
		       GizmoDirection == EGizmoDirection::YZ_Plane;
	}

	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }
	USceneComponent* GetTargetComponent() const { return TargetComponent; }
	void SetSelectedComponent(USceneComponent* InComponent) { TargetComponent = InComponent; }
	USceneComponent* GetSelectedComponent() const { return TargetComponent; }
	bool IsInRadius(float Radius) const;
	bool HasComponent() const { return TargetComponent; }
	FVector GetPreviousMouseLocation() const { return PreviousMouseLocation; }
	FVector2 GetPreviousScreenPos() const { return PreviousScreenPos; }
	FVector2 GetDragStartScreenPos() const { return DragStartScreenPos; }
	float GetCurrentRotationAngle() const { return CurrentRotationAngle; }
	float GetCurrentRotationAngleDegrees() const { return FVector::GetRadianToDegree(CurrentRotationAngle); }
	float GetSnappedRotationAngle(float SnapAngleDegrees) const
	{
		const float SnapAngleRadians = FVector::GetDegreeToRadian(SnapAngleDegrees);
		return std::round(CurrentRotationAngle / SnapAngleRadians) * SnapAngleRadians;
	}

	// 마우스 관련
	void EndDrag()
	{
		bIsDragging = false;
		CurrentRotationAngle = 0.0f;
	}
	bool IsDragging() const { return bIsDragging; }
	void OnMouseHovering() {}
	void OnMouseDragStart(const FVector& CollisionPoint);
	void OnMouseRelease(EGizmoDirection DirectionReleased) {}

private:
	TArray<FEditorPrimitive> Primitives;
	USceneComponent* TargetComponent = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;
	FQuaternion DragStartActorRotationQuat;
	FVector DragStartActorScale;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	bool bIsDragging = false;
	bool bIsWorld = true;	// Gizmo coordinate mode (true; world)

	FRenderState RenderState;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;

	// 회전 드래그 상태
	FVector PreviousMouseLocation;
	float CurrentRotationAngle = 0.0f;
	FVector DragStartDirection;

	// 스크린 공간 드래그 상태
	FVector2 DragStartScreenPos;      // 드래그 시작 시 스크린 좌표
	FVector2 PreviousScreenPos;       // 이전 프레임 스크린 좌표

	// Pilot Mode 고정 위치 & 본 뷰어 기즈모용
	bool bUseFixedLocation = false;
	FVector FixedLocation;

	// Location Snap 커스텀 설정 (뷰어용)
	bool bUseCustomLocationSnap = false;      // true면 ViewportManager 대신 커스텀 설정 사용
	bool bCustomLocationSnapEnabled = false;  // 커스텀 스냅 활성화 여부 (기본값: 비활성화)
	float CustomLocationSnapValue = 10.0f;    // 커스텀 스냅 값

	// Rotation Snap 커스텀 설정 (뷰어용)
	bool bUseCustomRotationSnap = false;      // true면 ViewportManager 대신 커스텀 설정 사용
	bool bCustomRotationSnapEnabled = false;  // 커스텀 스냅 활성화 여부
	float CustomRotationSnapAngle = 15.0f;    // 커스텀 스냅 각도

	// Scale Snap 커스텀 설정 (뷰어용)
	bool bUseCustomScaleSnap = false;         // true면 ViewportManager 대신 커스텀 설정 사용
	bool bCustomScaleSnapEnabled = false;     // 커스텀 스냅 활성화 여부
	float CustomScaleSnapValue = 1.0f;        // 커스텀 스냅 값

	// 렌더 시 하이라이트 색상 계산 (상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;

	// Modular rendering functions
	void RenderCenterSphere(const FEditorPrimitive& P, float RenderScale);
	void RenderTranslatePlanes(const FEditorPrimitive& P, const FQuaternion& BaseRot, float RenderScale);
	void RenderScalePlanes(const FEditorPrimitive& P, const FQuaternion& BaseRot, float RenderScale);
	void RenderRotationCircles(const FEditorPrimitive& P, const FQuaternion& AxisRotation,
	const FQuaternion& BaseRot, const FVector4& AxisColor, const FVector& BaseAxis0, const FVector& BaseAxis1, UCamera* InCamera);
	void RenderRotationQuarterRing(const FEditorPrimitive& P, const FQuaternion& BaseRot,
	EGizmoDirection Direction, UCamera* InCamera, const FVector& BaseAxis0, const FVector& BaseAxis1);
};
