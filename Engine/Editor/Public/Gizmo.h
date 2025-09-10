#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Core/Public/Object.h"

enum class EGizmoDirection
{
	Right,
	Up,
	Forward,
	None
};
class AActor;
class UObjectPicker;

class UGizmo : public UObject
{
public:
	UGizmo();
	~UGizmo() override;
	void RenderGizmo(AActor* Actor, UObjectPicker& ObjectPicker);

	FVector MoveGizmo(UObjectPicker& ObjectPicker);

	void SetGizmoDirection(EGizmoDirection Direction);
	void EndDrag();
	bool IsDragging();

	const EGizmoDirection GetGizmoDirection();
	const FVector& GetGizmoLocation();
	float GetGizmoRadius();
	float GetGizmoHeight();

	void OnMouseHovering();
	void OnMouseClick(FVector4& CollisionPoint);
	void OnMouseRelease(EGizmoDirection DirectionReleased);


private:
	FEditorPrimitive Primitive;
	TArray<FVertex>* VerticesGizmo;
	AActor* TargetActor = nullptr;
	FVector4 ForwardColor{ 0,0,1,1 };
	FVector4 RightColor{ 1,0,0,1 };
	FVector4 UpColor{ 0,1,0,1 };
	FVector DragStartActorLocation;
	FVector4 DragStartMouseLocation;

	float Radius = 0.04f;
	float Height = 0.9f;
	float Scale = 2.0f;
	float HoveringFactor = 0.8f;	//호버링시 색상에 곱해지는 값
	bool bIsDragging = false;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
};
