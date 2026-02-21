#include "pch.h"
#include "FXAAActor.h"
#include "FXAAComponent.h"

AFXAAActor::AFXAAActor()
{
    FXAAComponent = CreateDefaultSubobject<UFXAAComponent>(FName("FXAAComponent"));
    RootComponent = FXAAComponent;

    SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName("SpriteComponent"));
    if (SpriteComponent)
    {
        SpriteComponent->SetTexture(FString("Editor/Icon/S_AtmosphericHeightFog.dds"));
        SpriteComponent->SetRelativeLocation(RootComponent->GetWorldLocation());
        SpriteComponent->SetEditable(false);
        SpriteComponent->SetupAttachment(RootComponent);
    }

}

UObject* AFXAAActor::Duplicate()
{
    AFXAAActor* DuplicatedActor = NewObject<AFXAAActor>(*this);
    DuplicatedActor->SetName(GetName().ToString());

    if (DuplicatedActor->RootComponent)
    {
        TSet<UActorComponent*> ComponentList = DuplicatedActor->GetComponents();
        for (UActorComponent* Component : ComponentList)
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

void AFXAAActor::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
}
