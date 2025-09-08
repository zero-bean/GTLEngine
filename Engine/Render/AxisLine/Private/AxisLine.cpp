#include "pch.h"
#include "Render/AxisLine/Public/AxisLine.h"

UAxisLineComponent::UAxisLineComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Line;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
}
