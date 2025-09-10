#include "pch.h"
#include "Editor/Public/Grid.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"


UGrid::UGrid()
{
	URenderer& Renderer = URenderer::GetInstance();
	SetLineVertices();

	Primitive.NumVertices = static_cast<uint32>(LineVertices.size());
	Primitive.Color = FVector4(1, 1, 1, 0.2f);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		LineVertices.data(), Primitive.NumVertices * sizeof(FVertex));
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
}
UGrid::~UGrid()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
}

void UGrid::RenderGrid()
{
	URenderer& Renderer = URenderer::GetInstance();

	Renderer.RenderPrimitive(Primitive, Primitive.RenderState);

}
void UGrid::SetGridProperty(float InCellSize, int32 InNumLines)
{
	this->CellSize = InCellSize;
	this->NumLines = InNumLines;
}

void UGrid::SetLineVertices()
{
	float LineLength = CellSize * static_cast<float>(NumLines) / 2.f;

	for (int32 LineCount = -NumLines/2; LineCount < NumLines/2; ++LineCount) // z축 라인
	{
		if (LineCount == 0)
		{
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , -LineLength}, Primitive.Color });
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , 0.f}, Primitive.Color });
		}
		else
		{
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , -LineLength}, Primitive.Color });
			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , LineLength}, Primitive.Color });
		}
	}
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // x축 라인
	{
		if (LineCount == 0)
		{
			LineVertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
			LineVertices.push_back({ {0.f, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
		}
		else
		{
			LineVertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
			LineVertices.push_back({ {LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
		}
	}
}
