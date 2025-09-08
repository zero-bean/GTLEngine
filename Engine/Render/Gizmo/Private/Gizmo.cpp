#include "pch.h"
#include "Render/Gizmo/Public/Gizmo.h"

#include "Render/Gizmo/Public/GizmoArrow.h"

AGizmo::AGizmo()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	GizmoArrowR = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowRed");
	GizmoArrowR->SetColor({1.f, 0.f, 0.f, 1.f});
	GizmoArrowR->SetRelativeRotation({0.f, 89.99f, 0.f});
	GizmoArrowR->SetVisibility(false);

	GizmoArrowG = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowGreen");
	GizmoArrowG->SetColor({0.f, 1.f, 0.f, 1.f});
	GizmoArrowG->SetRelativeRotation({-89.99f, 0.f, 0.f});
	GizmoArrowG->SetVisibility(false);

	GizmoArrowB = CreateDefaultSubobject<UGizmoArrowComponent>("GizmoArrowBlue");
	GizmoArrowB->SetColor({0.f, 0.f, 1.f, 1.f});
	GizmoArrowB->SetRelativeRotation({0.f, 0.f, 0.f});
	GizmoArrowB->SetVisibility(false);
}

AGizmo::~AGizmo()
{
	// Actor 소멸자에 의한 Component 삭제
}

void AGizmo::SetTargetActor(AActor* InActor)
{
	TargetActor = InActor;

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
		SetActorLocation(TargetActor->GetActorLocation());
		GizmoArrowR->SetRelativeLocation(GetActorLocation());
		GizmoArrowG->SetRelativeLocation(GetActorLocation());
		GizmoArrowB->SetRelativeLocation(GetActorLocation());
	}
	SetActorRotation({0, 0, 0});
}

void AGizmo::EndPlay()
{
	AActor::EndPlay();
}
