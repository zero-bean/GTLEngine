#include "pch.h"
#include "Mesh/Public/ResourceManager.h"
#include "Mesh/Public/VertexDatas.h"
#include "Render/Public/Renderer.h"

IMPLEMENT_SINGLETON(UResourceManager)

UResourceManager::UResourceManager() = default;

UResourceManager::~UResourceManager() = default;

void UResourceManager::Initialize()
{
	URenderer& Renderer = URenderer::GetInstance();
	//TMap.Add()
	VertexDatas.emplace(EPrimitiveType::Cube, &VerticesCube);
	VertexDatas.emplace(EPrimitiveType::Sphere, &VerticesSphere);
	VertexDatas.emplace(EPrimitiveType::Triangle, &VerticesTriangle);
	VertexDatas.emplace(EPrimitiveType::Arrow, &VerticesArrow);

	//TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	Vertexbuffers.emplace(EPrimitiveType::Cube,
	                      Renderer.CreateVertexBuffer(VerticesCube.data(),
	                                                  static_cast<UINT>(VerticesCube.size()) * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere,
	                      Renderer.CreateVertexBuffer(VerticesSphere.data(),
	                                                  static_cast<UINT>(VerticesSphere.size()) * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Triangle,
	                      Renderer.CreateVertexBuffer(VerticesTriangle.data(),
	                                                  static_cast<UINT>(VerticesTriangle.size()) * sizeof(FVertex)));

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<UINT>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<UINT>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<UINT>(VerticesTriangle.size()));
}

void UResourceManager::Release()
{
	URenderer& Renderer = URenderer::GetInstance();
	//TMap.Value()
	for (auto& Pair : Vertexbuffers)
	{
		Renderer.ReleaseVertexBuffer(Pair.second);
	}
	//TMap.Empty()
	Vertexbuffers.clear();
}

TArray<FVertex>* UResourceManager::GetVertexData(EPrimitiveType Type)
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
