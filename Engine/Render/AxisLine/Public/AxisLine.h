#pragma once
#include "Mesh/Public/SceneComponent.h"

class UAxisLineComponent : public UPrimitiveComponent
{
public:
	void LoadMeshResource(const EPrimitiveType& InType);
};
