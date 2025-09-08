#pragma once
#include "Core/Public/Object.h"

class UActorComponent : public UObject
{
public:
	UActorComponent();
	~UActorComponent();
	/*virtual void Render(const URenderer& Renderer) const
	{

	}*/

	virtual void BeginPlay();
	virtual void TickComponent();
	virtual void EndPlay();

	EComponentType GetComponentType() const { return ComponentType; }
protected:
	EComponentType ComponentType;
private:
};
