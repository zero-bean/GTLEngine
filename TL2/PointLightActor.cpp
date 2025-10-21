#include "pch.h"
#include "PointLightActor.h"
#include "PointLightComponent.h"
#include "BillboardComponent.h"

APointLightActor::APointLightActor()
{
    Name = "Point Light";
    PointLightComponent = CreateDefaultSubobject<UPointLightComponent>("PointLightComponent");
    RootComponent = PointLightComponent;

    if (!SpriteComponent)
    {
        SpriteComponent = CreateDefaultSubobject<UBillboardComponent>("SpriteComponent");
        if (SpriteComponent)
        {
            SpriteComponent->SetTexture("Editor/Icon/PointLight_64x.dds");
            SpriteComponent->SetRelativeLocation(PointLightComponent->GetRelativeLocation());
            SpriteComponent->SetupAttachment(PointLightComponent);
            SpriteComponent->SetEditable(false);
        }        
    }
}

APointLightActor::~APointLightActor()
{
}

void APointLightActor::Tick(float DeltaTime)
{
}

UObject* APointLightActor::Duplicate()
{
    return AActor::Duplicate();
}

void APointLightActor::DuplicateSubObjects()
{
    AActor::DuplicateSubObjects();
}
