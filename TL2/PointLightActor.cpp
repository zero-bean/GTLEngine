#include "pch.h"
#include "PointLightActor.h"
#include "PointLightComponent.h"
#include "BillboardComponent.h"

APointLightActor::APointLightActor()
{
    Name = "Point Light";
    PointLightComponent = CreateDefaultSubobject<UPointLightComponent>("PointLightComponent");
    RootComponent = PointLightComponent;

    UBillboardComponent* BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
    if (BillboardComponent)
    {
        BillboardComponent->SetTexture("Editor/Icon/PointLight_64x.dds");
        BillboardComponent->SetRelativeLocation(RootComponent->GetRelativeLocation());
        BillboardComponent->SetupAttachment(PointLightComponent);
        BillboardComponent->SetEditable(false);
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
