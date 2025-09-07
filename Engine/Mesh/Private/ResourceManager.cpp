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

	VertexDatas.emplace(EPrimitiveType::LineR, &VerticesLineRed);
	VertexDatas.emplace(EPrimitiveType::LineG, &VerticesLineGreen);
	VertexDatas.emplace(EPrimitiveType::LineB, &VerticesLineBlue);

	VertexDatas.emplace(EPrimitiveType::GizmoR, &VerticesGizmoRed);
	VertexDatas.emplace(EPrimitiveType::GizmoG, &VerticesGizmoGreen);
	VertexDatas.emplace(EPrimitiveType::GizmoB, &VerticesGizmoBlue);

	//TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	Vertexbuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(
		                      VerticesCube.data(), static_cast<int>(VerticesCube.size()) * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(
		                      VerticesSphere.data(), static_cast<int>(VerticesSphere.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(
		                      VerticesTriangle.data(), static_cast<int>(VerticesTriangle.size() * sizeof(FVertex))));

	Vertexbuffers.emplace(EPrimitiveType::LineR, Renderer.CreateVertexBuffer(
		                      VerticesLineRed.data(), static_cast<int>(VerticesLineRed.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::LineG, Renderer.CreateVertexBuffer(
		                      VerticesLineGreen.data(), static_cast<int>(VerticesLineGreen.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::LineB, Renderer.CreateVertexBuffer(
		                      VerticesLineBlue.data(), static_cast<int>(VerticesLineBlue.size() * sizeof(FVertex))));

	Vertexbuffers.emplace(EPrimitiveType::GizmoR, Renderer.CreateVertexBuffer(
		                      VerticesGizmoRed.data(), static_cast<int>(VerticesGizmoRed.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::GizmoG, Renderer.CreateVertexBuffer(
		                      VerticesGizmoGreen.data(),
		                      static_cast<int>(VerticesGizmoGreen.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::GizmoB, Renderer.CreateVertexBuffer(
		                      VerticesGizmoBlue.data(), static_cast<int>(VerticesGizmoBlue.size() * sizeof(FVertex))));

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<UINT>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<UINT>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<UINT>(VerticesTriangle.size()));

	NumVertices.emplace(EPrimitiveType::LineR, static_cast<UINT>(VerticesLineRed.size()));
	NumVertices.emplace(EPrimitiveType::LineG, static_cast<UINT>(VerticesLineGreen.size()));
	NumVertices.emplace(EPrimitiveType::LineB, static_cast<UINT>(VerticesLineBlue.size()));

	NumVertices.emplace(EPrimitiveType::GizmoR, static_cast<UINT>(VerticesGizmoRed.size()));
	NumVertices.emplace(EPrimitiveType::GizmoG, static_cast<UINT>(VerticesGizmoGreen.size()));
	NumVertices.emplace(EPrimitiveType::GizmoB, static_cast<UINT>(VerticesGizmoBlue.size()));
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
