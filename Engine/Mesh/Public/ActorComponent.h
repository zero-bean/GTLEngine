#pragma once
#include "Core/Public/Object.h"
#include "Render/Public/Renderer.h"

class UActorComponent : public UObject
{
public:
	virtual void Render(const URenderer& Renderer) const
	{

	}
};
