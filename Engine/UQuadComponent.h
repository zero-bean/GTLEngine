#pragma once
#include "UPrimitiveComponent.h"
#include "Vector.h"

class UMeshManager;
class UMaterialManager;

class UQuadComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(UQuadComponent, UPrimitiveComponent)
private:
	bool IsManageable() override { return true; }

public:
	UQuadComponent(FVector pos = { 0, 0, 0 }, FVector rot = { 0, 0, 0 }, FVector scl = { 1, 1, 1 })
		: UPrimitiveComponent(pos, rot, scl) {}
};