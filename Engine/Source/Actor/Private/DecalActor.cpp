#include "pch.h"
#include "Actor/Public/DecalActor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Texture/Public/Material.h"

IMPLEMENT_CLASS(ADecalActor, AActor)

ADecalActor::ADecalActor()
{
	if (DecalComponent = CreateDefaultSubobject<UDecalComponent>(FName("DecalComponent")))
	{
		DecalComponent->SetParentAttachment(GetRootComponent());
		SetRootComponent(DecalComponent);
	}

	SetActorTickEnabled(true);
	SetTickInEditor(true);
}

ADecalActor::~ADecalActor()
{

}
