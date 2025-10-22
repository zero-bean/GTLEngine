// Recreated after accidental truncation
#include "pch.h"
#include "SpotLightActor.h"
#include "SpotLightComponent.h"
#include "BillboardComponent.h"

ASpotLightActor::ASpotLightActor()
{
    Name = "Spot Light Actor";
    SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(FName("SpotLightComponent"));
    RootComponent = SpotLightComponent;

    if (!SpriteComponent)
    {
        SpriteComponent = CreateDefaultSubobject<UBillboardComponent>("SpriteComponent");
        if (SpriteComponent)
        {
            SpriteComponent->SetTexture("Editor/Icon/PointLight_64x.dds");
            SpriteComponent->SetRelativeLocation(SpotLightComponent->GetRelativeLocation());
            SpriteComponent->SetupAttachment(SpotLightComponent);
            SpriteComponent->SetEditable(false);
        }        
    }
}

ASpotLightActor::~ASpotLightActor()
{
}

void ASpotLightActor::Tick(float DeltaTime)
{
    SpotLightComponent->SetRelativeRotation(GetActorRotation()); 

    FVector fwd = GetActorRotation().RotateVector(FVector(0, 0, 1));
    SpotLightComponent->SetDirection(fwd);
}

UObject* ASpotLightActor::Duplicate()
{
    return Super_t::Duplicate();
}

void ASpotLightActor::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();

    SpotLightComponent = Cast<USpotLightComponent>(RootComponent);    
}
