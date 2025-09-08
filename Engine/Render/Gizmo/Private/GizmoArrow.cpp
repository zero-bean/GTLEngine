#include "pch.h"
#include "Render/Gizmo/Public/GizmoArrow.h"

UGizmoArrowComponent::UGizmoArrowComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Gizmo;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}
