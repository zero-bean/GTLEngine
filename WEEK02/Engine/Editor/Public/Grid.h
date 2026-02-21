#pragma once
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include "Editor/Public/EditorPrimitive.h"

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
