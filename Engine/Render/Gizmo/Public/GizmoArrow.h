#pragma once
#include "Mesh/Public/SceneComponent.h"

class UGizmoArrowComponent : public UPrimitiveComponent
{
public:
	void LoadMeshResource(const EPrimitiveType& InType);
};

