#include "pch.h"
#include "Actor/Public/SpotLightActor.h"

#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/BillboardComponent.h"

IMPLEMENT_CLASS(ASpotLightActor, ALightActor)

ASpotLightActor::ASpotLightActor()
{
	auto SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>("SpotLightComponent");
	SpotLightComponent->SetOwner(this);
	SetRootComponent(SpotLightComponent);

	UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>("Billboard");
	Billboard->SetOwner(this);
	SpotLightComponent->AddChild(Billboard);

	Billboard->SetSprite(ELightType::Spotlight);
	Billboard->SetParentAttachment(SpotLightComponent);
	// jft
	Billboard->SetAboveActor();
}
