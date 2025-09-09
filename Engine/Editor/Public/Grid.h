#pragma once
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include <vector>

class UGrid : public UObject
{
public:
	UGrid();
	~UGrid() override;
	void SetLineVertices();
	void SetGridProperty(float InCellSize, int InNumLines);

	void RenderGrid();

private:
	float CellSize = 1.0f;
	int NumLines = 250;
	FEditorPrimitive Primitive;
	TArray<FVertex> LineVertices;
};

//cell 1 halfline 25
