#include "pch.h"
#include "DirectionalLightActor.h"
#include "BillboardComponent.h"
#include "GizmoArrowComponent.h"

ADirectionalLightActor::ADirectionalLightActor()
{
    Name = "Directional Light";
    DirectionalLightComponent = CreateDefaultSubobject<UDirectionalLightComponent>("DirectionalLightComponent");
    RootComponent = DirectionalLightComponent;

    UGizmoArrowComponent** DirectionComponent = DirectionalLightComponent->GetDirectionComponent();
    (*DirectionComponent) = CreateDefaultSubobject<UGizmoArrowComponent>("DirectionComponent");

    if (DirectionComponent)
    {
        (*DirectionComponent)->SetupAttachment(RootComponent);
        (*DirectionComponent)->SetActive(false);
        (*DirectionComponent)->SetWorldScale(FVector(0.25f, 0.25f, 1.0f));
        (*DirectionComponent)->SetEditable(false);
    }
    
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
