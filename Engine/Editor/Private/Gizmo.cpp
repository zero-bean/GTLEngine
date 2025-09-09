#include "pch.h"
#include "Editor/Public/Gizmo.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Mesh/Public/Actor.h"

UGizmo::UGizmo()
{

	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	
	VerticesGizmo = ResourceManager.GetVertexData(EPrimitiveType::Gizmo);
	Primitive.Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Gizmo);
	Primitive.NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Gizmo);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
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
	bIsVisible = true;
	if (TargetActor)
	{
		Primitive.Location = TargetActor->GetActorLocation();
		Primitive.Rotation = { 0,89.99f,0 };
		Primitive.Scale = TargetActor->GetActorScale3D()*2;
		Primitive.Color = FVector4(1, 0, 0, 1);
		Renderer.RenderPrimitive(Primitive);

		Primitive.Rotation = { -89.99f, 0, 0 };
		Primitive.Color = FVector4(0, 1, 0, 1);
		Renderer.RenderPrimitive(Primitive);

		Primitive.Rotation = { 0, 0, 0 };
		Primitive.Color = FVector4(0, 0, 1, 1);
		Renderer.RenderPrimitive(Primitive);
	}
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
