#include "../Engine/Mesh/Public/ResourceManager.h"
#include "../Engine/Mesh/Public/VertexDatas.h"
#include "../Engine/Render/Public/Renderer.h"



UResourceManager& UResourceManager::GetInstance()
{
	static UResourceManager Instance;
	return Instance;
}

void UResourceManager::Initialize(URenderer& Renderer)
{
	//TMap.Add()
	VertexDatas.emplace( EPrimitiveType::Cube, &VerticesCube );
	VertexDatas.emplace(EPrimitiveType::Sphere, &VerticesSphere);
	VertexDatas.emplace(EPrimitiveType::Triangle, &VerticesTriangle);

	//TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	Vertexbuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(VerticesCube.data(), VerticesCube.size()*sizeof(FVertexSimple)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(VerticesSphere.data(), VerticesSphere.size() * sizeof(FVertexSimple)));
	Vertexbuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(VerticesTriangle.data(), VerticesTriangle.size() * sizeof(FVertexSimple)));
	
	NumVertices.emplace(EPrimitiveType::Cube, VerticesCube.size());
	NumVertices.emplace(EPrimitiveType::Sphere, VerticesSphere.size());
	NumVertices.emplace(EPrimitiveType::Triangle, VerticesTriangle.size());
}

void UResourceManager::Release(URenderer& Renderer)
{
	//TMap.Value()
	for (auto& pair : Vertexbuffers)
	{
		Renderer.ReleaseVertexBuffer(pair.second);
	}
	//TMap.Empty()
	Vertexbuffers.clear();
}

TArray<FVertexSimple>* UResourceManager::GetVertexData(EPrimitiveType Type)
{
	return VertexDatas[Type];
}
ID3D11Buffer* UResourceManager::GetVertexbuffer(EPrimitiveType Type)
{
	return Vertexbuffers[Type];
}
UINT UResourceManager::GetNumVertices(EPrimitiveType Type)
{
	return NumVertices[Type];
}
