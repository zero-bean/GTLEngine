#pragma once
#include "StaticMeshActor.h"

class AAmbientActor : public AStaticMeshActor
{
public:
    DECLARE_SPAWNABLE_CLASS(AAmbientActor, AStaticMeshActor, "AmbientActor");
    AAmbientActor();
    ~AAmbientActor() override;

    //void Tick(float DeltaTime) override;

   UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    
};
