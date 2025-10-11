#include "pch.h"
#include "Actor/Public/SpotLightActor.h"

#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/BillboardComponent.h"

IMPLEMENT_CLASS(ASpotLightActor, ALightActor)

ASpotLightActor::ASpotLightActor()
{
	auto SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>("SpotLightComponent");
	SetRootComponent(SpotLightComponent);
	auto BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	BillboardComponent->SetSprite(ELightType::Spotlight);

	SpotLightComponent->AddChild(BillboardComponent);
	BillboardComponent->SetParentAttachment(SpotLightComponent);
}
