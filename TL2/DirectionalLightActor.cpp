#include "pch.h"
#include "DirectionalLightActor.h"
#include "BillboardComponent.h"
#include "GizmoArrowComponent.h"
#include "SceneRotationUtils.h"

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

    DirectionComponent = CreateDefaultSubobject<UGizmoArrowComponent>("DirectionComponent");
    if (DirectionComponent)
    {
        DirectionComponent->SetupAttachment(RootComponent);
        DirectionComponent->SetEditable(false);
        DirectionComponent->SetColor(FVector(1.0f, 0.f, 0.f));
        DirectionComponent->SetRelativeScale(FVector(0.15f, 0.15f, 0.7f));
        FQuat Rotation = SceneRotUtil::QuatFromEulerZYX_Deg(DirectionComponent->GetDirection());
        DirectionComponent->SetRelativeRotation(Rotation);
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
