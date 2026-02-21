// DecalActor.h
#pragma once
#include "Actor.h"
#include "DecalComponent.h"

class ADecalActor : public AActor
{
public:
    DECLARE_SPAWNABLE_CLASS(ADecalActor, AActor, "Decal")

    ADecalActor();
    virtual ~ADecalActor() override;

    virtual void Tick(float DeltaTime) override;

    UDecalComponent* GetDecalComponent() const { return DecalComponent; }
    void SetDecalComponent(UDecalComponent* InDecalComponent);

    virtual bool DeleteComponent(UActorComponent* ComponentToDelete) override;

    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    UDecalComponent* DecalComponent;
    UBillboardComponent* SpriteComponent;
};
