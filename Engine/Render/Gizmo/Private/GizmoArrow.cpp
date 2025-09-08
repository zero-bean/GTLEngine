#include "pch.h"
#include "Render/Gizmo/Public/GizmoArrow.h"

#include "Mesh/Public/Actor.h"

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
}

void UGizmoArrowComponent::MoveActor(float Distance)
{
	GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + Forward * Distance);
}

void UGizmoArrowComponent::OnReleased()
{
	bIsClicked = false;
}

void UGizmoArrowComponent::TickComponent()
{
	UPrimitiveComponent::TickComponent();

	if (bIsClicked)
	{
		//distance 받기..
		//MoveActor(distance);
	}
}
