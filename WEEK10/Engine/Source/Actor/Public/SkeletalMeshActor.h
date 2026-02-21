#pragma once
#include "Component/Mesh/Public/SkeletalMeshComponent.h"

UCLASS()
class ASkeletalMeshActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASkeletalMeshActor, AActor)

public:
	ASkeletalMeshActor();

	virtual UClass* GetDefaultRootComponent() override;

	USkeletalMeshComponent* GetSkeletalMeshComponent();

private:
	USkeletalMeshComponent* SkeletalMeshComponent;
};
