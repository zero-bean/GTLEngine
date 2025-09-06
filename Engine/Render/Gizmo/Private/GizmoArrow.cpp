#include "pch.h"
#include "Render/Gizmo/Public/GizmoArrow.h"

void UGizmoArrowComponent::LoadMeshResource(const EPrimitiveType& InType)
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = InType;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}
