#include "pch.h"
#include "ExponentialHeightFog.h"
#include "ExponentialHeightFogComponent.h"

AExponentialHeightFog::AExponentialHeightFog()
{
	HeightFogComponent = CreateDefaultSubobject<UExponentialHeightFogComponent>(FName("HeightFogComponent"));
    RootComponent = HeightFogComponent;

	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName("SpriteComponent"));
    if (SpriteComponent)
    {
        SpriteComponent->SetTexture(FString("Editor/Icon/S_AtmosphericHeightFog.dds"));
        SpriteComponent->SetRelativeLocation(RootComponent->GetWorldLocation());
        SpriteComponent->SetEditable(false);
        SpriteComponent->SetupAttachment(RootComponent);
    }

}

UObject* AExponentialHeightFog::Duplicate()
{

    AExponentialHeightFog* DuplicatedActor = NewObject<AExponentialHeightFog>(*this);

    DuplicatedActor->SetName(GetName().ToString());

    if (DuplicatedActor->RootComponent)
    {
        TSet<UActorComponent*> ComponentsToDelete = DuplicatedActor->GetComponents();
        for (UActorComponent* Component : ComponentsToDelete)
        {
            DuplicatedActor->OwnedComponents.Remove(Component);
            ObjectFactory::DeleteObject(Component);
        }
        DuplicatedActor->RootComponent = nullptr;
    }

    if (RootComponent)
    {
        DuplicatedActor->RootComponent = Cast<USceneComponent>(RootComponent->Duplicate());
    }
    DuplicatedActor->DuplicateSubObjects();

    return DuplicatedActor;
}

void AExponentialHeightFog::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}
