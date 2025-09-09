#include "pch.h"
#include "Mesh/Public/ResourceManager.h"
#include "Mesh/Public/VertexDatas.h"
#include "Render/Renderer/Public/Renderer.h"

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

	VertexDatas.emplace(EPrimitiveType::Line, &VerticesLine);
	VertexDatas.emplace(EPrimitiveType::Gizmo, &VerticesGizmo);

	//TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	Vertexbuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(
		                      VerticesCube.data(), static_cast<int>(VerticesCube.size()) * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(
		                      VerticesSphere.data(), static_cast<int>(VerticesSphere.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(
		                      VerticesTriangle.data(), static_cast<int>(VerticesTriangle.size() * sizeof(FVertex))));

	Vertexbuffers.emplace(EPrimitiveType::Line, Renderer.CreateVertexBuffer(
		                      VerticesLine.data(), static_cast<int>(VerticesLine.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Gizmo, Renderer.CreateVertexBuffer(
		                      VerticesGizmo.data(), static_cast<int>(VerticesGizmo.size() * sizeof(FVertex))));

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<uint32>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<uint32>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<uint32>(VerticesTriangle.size()));

	NumVertices.emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.size()));
	NumVertices.emplace(EPrimitiveType::Gizmo, static_cast<uint32>(VerticesGizmo.size()));
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

uint32 UResourceManager::GetNumVertices(EPrimitiveType Type)
{
	return NumVertices[Type];
}
