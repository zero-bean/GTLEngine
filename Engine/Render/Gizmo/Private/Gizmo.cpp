#include "pch.h"
#include "Render/Gizmo/Public/Gizmo.h"

#include "Render/Gizmo/Public/GizmoArrow.h"

AGizmo::AGizmo()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	GizmoArrowR = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowRed");
	GizmoArrowR->LoadMeshResource(EPrimitiveType::GizmoR);
	GizmoArrowG = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowGreen");
	GizmoArrowG->LoadMeshResource(EPrimitiveType::GizmoG);
	GizmoArrowB = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowBlue");
	GizmoArrowB->LoadMeshResource(EPrimitiveType::GizmoB);
}

void AGizmo::SetTargetActor(AActor* actor)
{
	TargetActor = actor;
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
