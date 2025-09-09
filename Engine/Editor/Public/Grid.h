#pragma once
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include <vector>

template<typename T>
using TArray = std::vector<T>;
class UGrid : public UObject
{
public:
	UGrid();
	~UGrid() override;
	void SetLineVertices();
	void SetGridProperty(float CellSize, int NumLines);
	
	void RenderGrid();

private:
	float CellSize = 1.0f;
	int NumLines = 50;
	FEditorPrimitive Primitive;
	TArray<FVertex> LineVertices;
};

//cell 1 halfline 25
