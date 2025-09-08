#pragma once
#include "Core/Public/Object.h"

class AActor;

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

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const {return Owner;}

	EComponentType GetComponentType() const { return ComponentType; }
protected:
	EComponentType ComponentType;
private:
	AActor* Owner;
};
