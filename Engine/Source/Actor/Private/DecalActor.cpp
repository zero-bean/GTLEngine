#include "pch.h"
#include "Actor/Public/DecalActor.h"
#include "Component/Public/DecalComponent.h"

IMPLEMENT_CLASS(ADecalActor, AActor)

ADecalActor::ADecalActor()
{
	UDecalComponent* DecalComponent = CreateDefaultSubobject<UDecalComponent>(FName("DecalComponent"));
	SetRootComponent(DecalComponent);

	SetActorTickEnabled(true);
	SetTickInEditor(true);
}

ADecalActor::~ADecalActor()
{

}

UDecalComponent* ADecalActor::GetDecalComponent() const
{
	return Cast<UDecalComponent>(GetRootComponent());
}
