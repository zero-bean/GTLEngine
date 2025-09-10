#include "pch.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/ObjectPicker.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Input/Public/InputManager.h"
#include "Mesh/Public/Actor.h"

UGizmo::UGizmo()
{

	UResourceManager& ResourceManager = UResourceManager::GetInstance();

	VerticesGizmo = ResourceManager.GetVertexData(EPrimitiveType::Gizmo);
	Primitive.Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Gizmo);
	Primitive.NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Gizmo);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitive.Scale = FVector(Scale, Scale, Scale);
	Primitive.bShouldAlwaysVisible = true;
	/*SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	GizmoArrowR = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowRed");
	GizmoArrowR->SetForward({ 1.f, 0.f, 0.f });
	GizmoArrowR->SetColor({1.f, 0.f, 0.f, 1.f});
	GizmoArrowR->SetRelativeRotation({0.f, 89.99f, 0.f});
	GizmoArrowR->SetVisibility(false);

	GizmoArrowG = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowGreen");
	GizmoArrowG->SetForward({ 0.f, 1.f, 0.f });
	GizmoArrowG->SetColor({0.f, 1.f, 0.f, 1.f});
	GizmoArrowG->SetRelativeRotation({-89.99f, 0.f, 0.f});
	GizmoArrowG->SetVisibility(false);

	GizmoArrowB = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowBlue");
	GizmoArrowB->SetForward({ 0.f, 0.f, 1.f });
	GizmoArrowB->SetColor({0.f, 0.f, 1.f, 1.f});
	GizmoArrowB->SetRelativeRotation({0.f, 0.f, 0.f});
	GizmoArrowB->SetVisibility(false);

	SetActorRotation({0.f,0.f,0.f});*/
}

UGizmo::~UGizmo() = default;

void UGizmo::RenderGizmo(AActor* Actor)
{

	TargetActor = Actor;
	URenderer& Renderer = URenderer::GetInstance();
	if (TargetActor)
	{
		FVector DistanceVector{ 0,0,0 };
		
		Primitive.Location = TargetActor->GetActorLocation();
		Primitive.Rotation = { 0,89.99f,0 };
		Primitive.Color = RightColor;
		Renderer.RenderPrimitive(Primitive);

		Primitive.Rotation = { -89.99f, 0, 0 };
		Primitive.Color = UpColor;
		Renderer.RenderPrimitive(Primitive);

		Primitive.Rotation = { 0, 0, 0 };
		Primitive.Color = ForwardColor;
		Renderer.RenderPrimitive(Primitive);
	}
}

void UGizmo::SetLocation(const FVector& Location)
{
	TargetActor->SetActorLocation(Location);
}

void UGizmo::SetGizmoDirection(EGizmoDirection Direction)
{
	GizmoDirection = Direction;
}

void UGizmo::EndDrag()
{
	bIsDragging = false;
}

bool UGizmo::IsDragging()
{
	return bIsDragging;
}

const EGizmoDirection UGizmo::GetGizmoDirection()
{
	return GizmoDirection;
}

const FVector& UGizmo::GetGizmoLocation()
{
	return Primitive.Location;
}

const FVector& UGizmo::GetDragStartMouseLocation()
{
	return DragStartMouseLocation;
}

const FVector& UGizmo::GetDragStartActorLocation()
{
	return DragStartActorLocation;
}

float UGizmo::GetGizmoRadius()
{
	return Radius*Scale;
}

float UGizmo::GetGizmoHeight()
{
	return Height*Scale;
}

void UGizmo::OnMouseRelease(EGizmoDirection DirectionReleased)
{
	switch (DirectionReleased)
	{
	case EGizmoDirection::Forward:
		ForwardColor.Z = 1.0f; break;
	case EGizmoDirection::Right:
		RightColor.X = 1.0f; break;
	case EGizmoDirection::Up:
		UpColor.Y = 1.0f;
	}
}


void UGizmo::OnMouseHovering()
{
	switch (GizmoDirection)
	{
	case EGizmoDirection::Forward:
		ForwardColor.Z = HoveringFactor; break;
	case EGizmoDirection::Right:
		RightColor.X = HoveringFactor; break;
	case EGizmoDirection::Up:
		UpColor.Y = HoveringFactor;
	}
}

void UGizmo::OnMouseDragStart(FVector& CollisionPoint)
{
	bIsDragging = true;
	DragStartMouseLocation = CollisionPoint;
	DragStartActorLocation = Primitive.Location;
}

//void AGizmo::SetTargetActor(AActor* actor)
//{
//	TargetActor = actor;
//	if (TargetActor)
//	{
//		SetActorLocation(TargetActor->GetActorLocation());
//	}
//	for (auto& Component : GetOwnedComponents())
//	{
//		if (Component->GetComponentType() >= EComponentType::Primitive)
//		{
//			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
//			if (TargetActor)
//			{
//				PrimitiveComponent->SetVisibility(true);
//			}
//			else
//			{
//				PrimitiveComponent->SetVisibility(false);
//			}
//		}
//	}
//}
//
//void AGizmo::BeginPlay()
//{
//	AActor::BeginPlay();
//}
//
//void AGizmo::Tick()
//{
//	AActor::Tick();
//
//	if (TargetActor)
//	{
//		SetActorLocation(TargetActor->GetActorLocation());
//
//		////Deprecated
//		//GizmoArrowR->SetRelativeLocation(GetActorLocation());
//		//GizmoArrowG->SetRelativeLocation(GetActorLocation());
//		//GizmoArrowB->SetRelativeLocation(GetActorLocation());
//	}
//	if (bIsWorld)
//	{
//		SetActorRotation({0, 0, 0});
//	}
//	else
//	{
//		//... 로컬 기준 기즈모 정렬
//
//	}
//}
//
//void AGizmo::EndPlay()
//{
//	AActor::EndPlay();
//}
