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
	virtual void TickComponent(float DeltaTime);
	virtual void EndPlay();
private:

};
