#include "pch.h"
#include "Editor/Public/Grid.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"


UGrid::UGrid()
{
	URenderer& Renderer = URenderer::GetInstance();
	SetLineVertices();
	
	Primitive.NumVertices = static_cast<int>(LineVertices.size()) * sizeof(FVertex);
	Primitive.Color = FVector4(1, 1, 1, 1);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		LineVertices.data(), Primitive.NumVertices);

}
UGrid::~UGrid()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
}

void UGrid::RenderGrid()
{
	URenderer& Renderer = URenderer::GetInstance();

	//Renderer.RenderPrimitive(Primitive);

}
void UGrid::SetGridProperty(float CellSize, int NumLines)
{
	this->CellSize = CellSize;
	this->NumLines = NumLines;
}

void UGrid::SetLineVertices()
{
	float LineLength = CellSize * NumLines;

	for (int LineCount = -NumLines/2; LineCount < NumLines/2; LineCount++) // z축 라인
	{
		LineVertices.push_back({ {LineCount * CellSize,0.0f , -LineLength}, Primitive.Color });
		LineVertices.push_back({ {LineCount * CellSize,0.0f , LineLength}, Primitive.Color });
	}
	for (int LineCount = -NumLines / 2; LineCount < NumLines / 2; LineCount++) // x축 라인
	{
		LineVertices.push_back({ {-LineLength, 0.0f, LineCount * CellSize}, Primitive.Color });
		LineVertices.push_back({ {LineLength, 0.0f, LineCount * CellSize}, Primitive.Color });
	}
}
