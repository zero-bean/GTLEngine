#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include <vector>


class AActor;

class UGizmo : public UObject
{
public:
	UGizmo();
	~UGizmo() override;
	void RenderGizmo(AActor* Actor);


private:
	FEditorPrimitive Primitive;
	TArray<FVertex>* VerticesGizmo;
	AActor* TargetActor = nullptr;
	bool bIsVisible = false;
};
