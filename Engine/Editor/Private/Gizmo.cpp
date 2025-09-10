#include "pch.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/ObjectPicker.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Input/Public/InputManager.h"
#include "Mesh/Public/Actor.h"

UGizmo::UGizmo()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Primitives.resize(3);
	GizmoColor.resize(3);

	/* *
	* @brief 0: Right, 1: Up, 2: Forward
	*/
	GizmoColor[0] = FVector4(1, 0, 0, 1);
	GizmoColor[1] = FVector4(0, 1, 0, 1);
	GizmoColor[2] = FVector4(0, 0, 1, 1);

	/* *
	* @brief Translation Setting
	*/
	const float ScaleT = TranslateCollisionConfig.Scale;
	Primitives[0].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/* *
	* @brief Rotation Setting
	*/
	Primitives[1].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Ring);
	Primitives[1].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Ring);
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/* *
	* @brief Scale Setting
	*/
	Primitives[2].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::CubeArrow);
	Primitives[2].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[2].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[2].bShouldAlwaysVisible = true;

	/* *
	* @brief Render State
	*/
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UGizmo::~UGizmo() = default;

void UGizmo::RenderGizmo(AActor* Actor)
{
	TargetActor = Actor;
	if (!TargetActor) { return; }

	URenderer& Renderer = URenderer::GetInstance();
	const int Mode = static_cast<int>(GizmoMode);
	auto& P = Primitives[Mode];
	P.Location = TargetActor->GetActorLocation();

	FVector LocalRotation{ 0,0,0 };
	//로컬 기즈모. 쿼터니언 구현후 사용
	/*if (!bIsWorld && TargetActor)
	{
		LocalRotation = TargetActor->GetActorRotation();
	}*/
	// X (Right)
	P.Rotation = FVector{0,89.99f,0} + LocalRotation;
	P.Color = ColorFor(EGizmoDirection::Right);
	Renderer.RenderPrimitive(P, RenderState);

	// Y (Up)
	P.Rotation = FVector{ -89.99f,0,0 } + LocalRotation;
	P.Color = ColorFor(EGizmoDirection::Up);
	Renderer.RenderPrimitive(P, RenderState);

	// Z (Forward)
	P.Rotation = FVector{ 0, 0, 0 } + LocalRotation;
	P.Color = ColorFor(EGizmoDirection::Forward);
	Renderer.RenderPrimitive(P, RenderState);
}

void UGizmo::ChangeGizmoMode()
{
	switch (GizmoMode)
	{
	case EGizmoMode::Translate:
		GizmoMode = EGizmoMode::Rotate; break;
	case EGizmoMode::Rotate:
		GizmoMode = EGizmoMode::Scale; break;
	case EGizmoMode::Scale:
		GizmoMode = EGizmoMode::Translate; 
	}
}

void UGizmo::SetLocation(const FVector& Location)
{
	TargetActor->SetActorLocation(Location);
}

bool UGizmo::IsInRadius(float Radius)
{
	if (Radius >= RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale && Radius <= RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale)
		return true;
	return false;
}

void UGizmo::OnMouseDragStart(FVector& CollisionPoint)
{
	bIsDragging = true;
	DragStartMouseLocation = CollisionPoint;
	DragStartActorLocation = Primitives[(int)GizmoMode].Location;
	DragStartActorRotation = TargetActor->GetActorRotation();
	DragStartActorScale = TargetActor->GetActorScale3D();
}

// 하이라이트 색상은 렌더 시점에만 계산 (상태 오염 방지)
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis) const
{
	const int Idx = AxisIndex(InAxis);
	const FVector4& BaseColor = GizmoColor[Idx];
	const bool bIsHighlight = (InAxis == GizmoDirection);

	const FVector4 Paint = bIsHighlight ? FVector4(1,1,0,1) : BaseColor;

	if (bIsDragging)
		return BaseColor;
	else
		return Paint;
}
