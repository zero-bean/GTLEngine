#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Core/Public/Object.h"
#include "Mesh/Public/Actor.h"

class UObjectPicker;

enum class EGizmoMode
{
	Translate,
	Rotate,
	Scale
};

enum class EGizmoDirection
{
	Right,
	Up,
	Forward,
	None
};

struct FGizmoTranslationCollisionConfig
{
	FGizmoTranslationCollisionConfig()
		: Radius(0.04f), Height(0.9f), Scale(2.f) {
	}

	float Radius = {};
	float Height = {};
	float Scale = {};
};

struct FGizmoRotateCollisionConfig
{
	FGizmoRotateCollisionConfig()
		: OuterRadius(1.0f), InnerRadius(0.9f), Scale(2.f) {
	}

	float OuterRadius = {1.0f};  // 링 큰 반지름 
	float InnerRadius = {0.9f};  // 링 굵기 r  
	float Scale = {2.0f};
};

class UGizmo : public UObject
{
public:
	UGizmo();
	~UGizmo() override;
	void RenderGizmo(AActor* Actor);
	void ChangeGizmoMode();

	/* *
	* @brief Setter
	*/
	void SetLocation(const FVector& Location);
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }
	void SetActorRotation(const FVector& Rotation) { TargetActor->SetActorRotation(Rotation); }

	/* *
	* @brief Getter
	*/
	const EGizmoDirection GetGizmoDirection() { return GizmoDirection; }
	const FVector& GetGizmoLocation() { return Primitives[(int)GizmoMode].Location; }
	const FVector& GetActorRotation() { return TargetActor->GetActorRotation(); }
	const FVector& GetDragStartMouseLocation() { return DragStartMouseLocation; }
	const FVector& GetDragStartActorLocation() { return DragStartActorLocation; }
	const FVector& GetDragStartActorRotation() { return DragStartActorRotation; }
	const EGizmoMode GetGizmoMode() { return GizmoMode; }

	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }
	bool IsInRadius(float Radius);

	/* *
	* @brief 마우스 관련
	*/
	void EndDrag() { bIsDragging = false; }
	bool IsDragging() const { return bIsDragging; }
	void OnMouseHovering() {}
	void OnMouseDragStart(FVector& CollisionPoint);
	void OnMouseRelease(EGizmoDirection DirectionReleased) {}

private:
	static inline int AxisIndex(EGizmoDirection InDirection)
	{
		switch (InDirection)
		{
		case EGizmoDirection::Right:   return 0;
		case EGizmoDirection::Up:      return 1;
		case EGizmoDirection::Forward: return 2;
		default:                       return 2;
		}
	}

	// 렌더 시 하이라이트 색상 계산(상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;
	

	TArray<FEditorPrimitive> Primitives;
	AActor* TargetActor = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	float HoveringFactor = 0.8f;
	bool bIsDragging = false;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;
};
