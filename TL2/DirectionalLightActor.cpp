#include "pch.h"
#include "DirectionalLightActor.h"
#include "BillboardComponent.h"

ADirectionalLightActor::ADirectionalLightActor()
{
    Name = "Directional Light";
    DirectionalLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>("DirectionalLightComponent");
    RootComponent = DirectionalLightComponent;
    
    if (!SpriteComponent)
    {
        SpriteComponent = CreateDefaultSubobject<UBillboardComponent>("SpriteComponent");
        if (SpriteComponent)
        {
            SpriteComponent->SetTexture("Editor/Icon/PointLight_64x.dds");
            SpriteComponent->SetRelativeLocation(DirectionalLightComponent->GetRelativeLocation());
            SpriteComponent->SetupAttachment(DirectionalLightComponent);
            SpriteComponent->SetEditable(false);
        }        
    }
}

ADirectionalLightActor::~ADirectionalLightActor()
{
}

void ADirectionalLightActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

UObject* ADirectionalLightActor::Duplicate()
{
    return AActor::Duplicate();
}

void ADirectionalLightActor::DuplicateSubObjects()
{
    AActor::DuplicateSubObjects();
}
