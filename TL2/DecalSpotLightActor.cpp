#include "pch.h"
#include "DecalSpotLightActor.h"
#include "ObjectFactory.h"

ADecalSpotLightActor::ADecalSpotLightActor()
{
    DecalSpotLightComponent = CreateDefaultSubobject<UDecalSpotLightComponent>(FName("DecalSpotLightComponent"));
    RootComponent = DecalSpotLightComponent;

    SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName("SpriteComponent"));
    if (SpriteComponent)
    {
        SpriteComponent->SetTexture(FString("Editor/Icon/SpotLight_64x.dds"));
        SpriteComponent->SetRelativeLocation(RootComponent->GetWorldLocation());
        SpriteComponent->SetEditable(false);
        SpriteComponent->SetupAttachment(RootComponent);
    }
}

ADecalSpotLightActor::~ADecalSpotLightActor()
{
}
void ADecalSpotLightActor::Tick(float DeltaTime)
{
    SpriteComponent->SetTexture(FString("Editor/Icon/SpotLight_64x.dds"));
}

UObject* ADecalSpotLightActor::Duplicate()
{
    // 부모 클래스의 Duplicate 호출 (RootComponent와 모든 자식 컴포넌트 복제)
    ADecalSpotLightActor* NewActor = static_cast<ADecalSpotLightActor*>(AActor::Duplicate());
    return NewActor;
}

void ADecalSpotLightActor::DuplicateSubObjects()
{
    // 부모 클래스가 OwnedComponents를 재구성
    AActor::DuplicateSubObjects();

    // OwnedComponents를 순회하면서 각 타입의 컴포넌트를 찾아 포인터 재설정
    for (UActorComponent* Component : OwnedComponents)
    {
        if (UDecalSpotLightComponent* SpotLight = Cast<UDecalSpotLightComponent>(Component))
        {
            DecalSpotLightComponent = SpotLight;
        }
        else if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(Component))
        {
            SpriteComponent = Billboard;
        }
    }
}
