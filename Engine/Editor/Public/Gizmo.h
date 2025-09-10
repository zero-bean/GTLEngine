#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Core/Public/Object.h"

class AActor;
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
		: OuterRadius(0.05f), InnerRadius(1.2f), Scale(2.f) {
	}

	float OuterRadius = {};  // 링 큰 반지름 
	float InnerRadius = {};  // 링 굵기 r  
	float Scale = {};
};

class UGizmo : public UObject
{
public:
	UGizmo();
	~UGizmo() override;
	void RenderGizmo(AActor* Actor);

	/* *
	* @brief Setter
	*/
	void SetLocation(const FVector& Location);
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }

	/* *
	* @brief Getter
	*/
	const EGizmoDirection GetGizmoDirection() const { return GizmoDirection; }
	const FVector& GetGizmoLocation() { return Primitives[(int)GizmoMode].Location; }
	const FVector& GetDragStartMouseLocation() { return DragStartMouseLocation; }
	const FVector& GetDragStartActorLocation() { return DragStartActorLocation; }
	// Translation 전용 
	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	// Rotation 전용
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }

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
	TArray<TArray<FVertex>*> GizmoVertices;
	AActor* TargetActor = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig      RotateCollisionConfig;
	float HoveringFactor = 0.8f;
	bool bIsDragging = false;

	FRenderState RenderState = {};

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;
};
