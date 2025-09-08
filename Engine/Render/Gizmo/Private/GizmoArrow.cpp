#include "pch.h"
#include "Render/Gizmo/Public/GizmoArrow.h"

#include "Manager/Input/Public/InputManager.h"
#include "Mesh/Public/Actor.h"
#include "Render/Gizmo/Public/Gizmo.h"

UGizmoArrowComponent::UGizmoArrowComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Gizmo;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}

void UGizmoArrowComponent::OnClicked()
{
	bIsClicked = true;
	SetColor({1.f, 1.f, 0.f, 1.f});
	DragStartLocation = GetOwner()->GetActorLocation();
}

void UGizmoArrowComponent::MoveActor(const FVector& Location)
{
	AGizmo* Gizmo = dynamic_cast<AGizmo*>(GetOwner());
	Gizmo->GetTargetActor()->SetActorLocation(Location);
}

void UGizmoArrowComponent::OnReleased()
{
	bIsClicked = false;
	SetColor(DefaultColor);
	DragStartLocation = FVector(0, 0, 0);
}

void UGizmoArrowComponent::TickComponent()
{
	UPrimitiveComponent::TickComponent();

	if (bIsClicked)
	{
		if (UInputManager::GetInstance().IsKeyDown(EKeyInput::MouseLeft))
		{
			OnReleased();
			return;
		}

		//DragStart
	}
}
