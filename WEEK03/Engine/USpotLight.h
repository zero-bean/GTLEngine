#pragma once
#include "stdafx.h"
#include "URenderer.h"
#include "UPrimitiveComponent.h"
#include "VertexPosColor.h"
#include "Vector.h"

class USpotLight : public UPrimitiveComponent
{
	DECLARE_UCLASS(USpotLight, UPrimitiveComponent)
private:
	bool IsManageable() override { return true; }
public:
	USpotLight(FVector pos = { 0, 0, 0 }, FVector rot = { 0, 0, 0 }, FVector scl = { 1, 1, 1 })
		:UPrimitiveComponent(pos, rot, scl)
	{
	}
};