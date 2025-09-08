#include "pch.h"
#include "Render/Gizmo/Public/Gizmo.h"

#include "Render/Gizmo/Public/GizmoArrow.h"

AGizmo::AGizmo()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	GizmoArrowR = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowRed");
	GizmoArrowR->LoadMeshResource(EPrimitiveType::GizmoR);
	GizmoArrowR->SetVisibility(false);
	GizmoArrowG = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowGreen");
	GizmoArrowG->LoadMeshResource(EPrimitiveType::GizmoG);
	GizmoArrowG->SetVisibility(false);
	GizmoArrowB = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowBlue");
	GizmoArrowB->LoadMeshResource(EPrimitiveType::GizmoB);
	GizmoArrowB->SetVisibility(false);
}

void AGizmo::SetTargetActor(AActor* actor)
{
	TargetActor = actor;

	for (auto& Component: GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
			if (TargetActor)
			{
				PrimitiveComponent->SetVisibility(true);
			}
			else
			{
				PrimitiveComponent->SetVisibility(false);
			}
		}
	}
}

void AGizmo::BeginPlay()
{
	AActor::BeginPlay();
}

void AGizmo::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	if (TargetActor)
	{
		SetActorLocation(TargetActor->GetActorLocation());
		GizmoArrowR->SetRelativeLocation(GetActorLocation());
		GizmoArrowG->SetRelativeLocation(GetActorLocation());
		GizmoArrowB->SetRelativeLocation(GetActorLocation());
	}
	SetActorRotation({0,0,0});
}

void AGizmo::EndPlay()
{
	AActor::EndPlay();
}
