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
	Vertexbuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(VerticesCube.data(), VerticesCube.size()*sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(VerticesSphere.data(), VerticesSphere.size() * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(VerticesTriangle.data(), VerticesTriangle.size() * sizeof(FVertex)));

	Vertexbuffers.emplace(EPrimitiveType::LineR, Renderer.CreateVertexBuffer(VerticesLineRed.data(), VerticesLineRed.size() * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::LineG, Renderer.CreateVertexBuffer(VerticesLineGreen.data(), VerticesLineGreen.size() * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::LineB, Renderer.CreateVertexBuffer(VerticesLineBlue.data(), VerticesLineBlue.size() * sizeof(FVertex)));

	Vertexbuffers.emplace(EPrimitiveType::GizmoR, Renderer.CreateVertexBuffer(VerticesGizmoRed.data(), VerticesGizmoRed.size() * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::GizmoG, Renderer.CreateVertexBuffer(VerticesGizmoGreen.data(), VerticesGizmoGreen.size() * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::GizmoB, Renderer.CreateVertexBuffer(VerticesGizmoBlue.data(), VerticesGizmoBlue.size() * sizeof(FVertex)));

	NumVertices.emplace(EPrimitiveType::Cube, VerticesCube.size());
	NumVertices.emplace(EPrimitiveType::Sphere, VerticesSphere.size());
	NumVertices.emplace(EPrimitiveType::Triangle, VerticesTriangle.size());

	NumVertices.emplace(EPrimitiveType::LineR, VerticesLineRed.size());
	NumVertices.emplace(EPrimitiveType::LineG, VerticesLineGreen.size());
	NumVertices.emplace(EPrimitiveType::LineB, VerticesLineBlue.size());

	NumVertices.emplace(EPrimitiveType::GizmoR, VerticesGizmoRed.size());
	NumVertices.emplace(EPrimitiveType::GizmoG, VerticesGizmoGreen.size());
	NumVertices.emplace(EPrimitiveType::GizmoB, VerticesGizmoBlue.size());
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
