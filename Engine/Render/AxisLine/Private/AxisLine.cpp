#include "pch.h"
#include "Render/AxisLine/Public/AxisLine.h"

void UAxisLineComponent::LoadMeshResource(const EPrimitiveType& InType)
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = InType;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}
