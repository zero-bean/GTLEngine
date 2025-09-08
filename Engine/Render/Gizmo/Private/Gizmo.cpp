#include "pch.h"
#include "Render/Gizmo/Public/Gizmo.h"

#include "Render/Gizmo/Public/GizmoArrow.h"

AGizmo::AGizmo()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

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

	SetActorRotation({0.f,0.f,0.f});
}

AGizmo::~AGizmo()
{
	// Actor 소멸자에 의한 Component 삭제
}

void AGizmo::SetTargetActor(AActor* actor)
{
	TargetActor = actor;
	if (TargetActor)
	{
		SetActorLocation(TargetActor->GetActorLocation());
	}
	for (auto& Component : GetOwnedComponents())
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

void AGizmo::Tick()
{
	AActor::Tick();

	if (TargetActor)
	{
		GizmoArrowR->SetRelativeLocation(GetActorLocation());
		GizmoArrowG->SetRelativeLocation(GetActorLocation());
		GizmoArrowB->SetRelativeLocation(GetActorLocation());
		TargetActor->SetActorLocation(GetActorLocation());
	}
	if (bIsWorld)
	{
		SetActorRotation({0, 0, 0});
	}
	else
	{
		//... 로컬 기준 기즈모 정렬

	}
}

void AGizmo::EndPlay()
{
	AActor::EndPlay();
}
