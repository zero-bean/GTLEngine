#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"
#include <vector>

template<typename T>
using TArray = std::vector<T>;
class UAxis : public UObject
{
public:
	UAxis();
	~UAxis() override;
	void Render();

private:
	FEditorPrimitive Primitive;
	TArray<FVertex> AxisVertices;
};
