#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"

UBatchLines::UBatchLines()
	: Grid()
	, BoundingBoxLines()
{
	Vertices.reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.resize(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	URenderer& Renderer = URenderer::GetInstance();

	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.NumIndices = static_cast<uint32>(Indices.size());
	Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), Primitive.NumIndices * sizeof(uint32));
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		Vertices.data(), Primitive.NumVertices * sizeof(FVector), true);
	Primitive.VertexShader = UAssetManager::GetInstance().GetVertexShader(EShaderType::BatchLine);
	Primitive.InputLayout = UAssetManager::GetInstance().GetIputLayout(EShaderType::BatchLine);
	Primitive.PixelShader = UAssetManager::GetInstance().GetPixelShader(EShaderType::BatchLine);
}

UBatchLines::~UBatchLines()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
	Primitive.InputLayout->Release();
	Primitive.VertexShader->Release();
	Primitive.IndexBuffer->Release();
	Primitive.PixelShader->Release();
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize())
	{
		return;
	}
	Grid.UpdateVerticesBy(newCellSize);
	Grid.MergeVerticesAt(Vertices, 0);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const IBoundingVolume& newBoundingVolumeInfo)
{
	switch (newBoundingVolumeInfo.GetType())
	{
	case EBoundingVolumeType::AABB:
		{
			const FAABB& box = static_cast<const FAABB&>(newBoundingVolumeInfo);
			if (BoundingBoxLines.GetRenderedAABBoxInfo().Min == box.Min && BoundingBoxLines.GetRenderedAABBoxInfo().Max == box.Max)
				return;

			BoundingBoxLines.UpdateVertices(box);
			BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
			bChangedVertices = true;
			break;
		}

	case EBoundingVolumeType::OBB:
		{
			const FOBB& box = static_cast<const FOBB&>(newBoundingVolumeInfo);
			// Early-out if unchanged
			if (BoundingBoxLines.GetRenderedOBBoxInfo().Center  == box.Center &&
				BoundingBoxLines.GetRenderedOBBoxInfo().Extents == box.Extents &&
				FMatrix::MatrixNearEqual(BoundingBoxLines.GetRenderedOBBoxInfo().Orientation, box.Orientation))
				return;

			BoundingBoxLines.UpdateVertices(box);
			BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
			bChangedVertices = true;
			break;
		}

	default:
		// None / Sphere not supported in this visualizer (yet)
		return;
	}
}

void UBatchLines::UpdateBatchLineVertices(const float newCellSize, const FAABB& newBoundingBoxInfo)
{
	UpdateUGridVertices(newCellSize);
	UpdateBoundingBoxVertices(newBoundingBoxInfo);
}

void UBatchLines::UpdateVertexBuffer()
{
	if (bChangedVertices)
	{
		URenderer::GetInstance().UpdateVertexBuffer(Primitive.Vertexbuffer, Vertices);
	}

	bChangedVertices = false;
}

void UBatchLines::Render(UPipeline& InPipeline)
{
	URenderer& Renderer = URenderer::GetInstance();

	// to do: 아래 함수를 batch에 맞게 수정해야 함.
	Renderer.RenderEditorPrimitiveIndexed(InPipeline, Primitive, Primitive.RenderState, false, sizeof(FVector), sizeof(uint32));
}

void UBatchLines::SetIndices()
{
	Indices.clear();
	const uint32 numGridVertices = Grid.GetNumVertices();
	Indices.reserve(numGridVertices + 24);

	for (uint32 index = 0; index < numGridVertices; ++index)
	{
		Indices.push_back(index);
	}

	uint32 boundingBoxLineIdx[] = {
		// 앞면
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// 뒷면
		4, 5,
		5, 6,
		6, 7,
		7, 4,

		// 옆면 연결
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
	{
		Indices.push_back(numGridVertices + boundingBoxLineIdx[i]);
	}
}

