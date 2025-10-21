// Recreated after accidental truncation
#include "pch.h"
#include "SpotLightActor.h"
#include "SpotLightComponent.h"
#include "BillboardComponent.h"

ASpotLightActor::ASpotLightActor()
{
    Name = "Spot Light Actor";
    SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(FName("SpotLightComponent"));

    // Editor billboard for spotlight visualization
    SpotBillboard = CreateDefaultSubobject<UBillboardComponent>(FName("SpotLightBillboard"));
    if (SpotBillboard)
    {
        SpotBillboard->SetTexture("Editor/Icon/SpotLight_64x.dds");
        SpotBillboard->SetBillboardSize(0.6f);
        SpotBillboard->SetupAttachment(SpotLightComponent);
        SpotBillboard->SetRelativeLocation(FVector(0, 0, 0));
        SpotBillboard->SetScreenSizeScaled(true);
        SpotBillboard->SetScreenSize(0.0025f);
        SpotBillboard->SetEditable(false);
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
    return nullptr;
}

void ASpotLightActor::DuplicateSubObjects()
{
}
