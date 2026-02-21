#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"
#include "Render/Renderer/Public/Pipeline.h"

class UAxis : public UObject
{
public:
	UAxis();
	~UAxis() override;
	void Render(UPipeline& InPipeline);

private:
	FEditorPrimitive Primitive;
	TArray<FNormalVertex> AxisVertices;
};
