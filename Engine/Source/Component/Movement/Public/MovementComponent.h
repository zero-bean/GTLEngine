#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Quaternion.h"

UCLASS()

class UMovementComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UMovementComponent, UActorComponent)

public:
	TObjectPtr<USceneComponent> UpdatedComponent;
	TObjectPtr<UPrimitiveComponent> UpdatedPrimitive;

    FVector Velocity;

    void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);

	void BeginPlay() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

    void UpdateComponentVelocity();
    void MoveUpdatedComponent(const FVector& Delta, const FQuaternion& NewRotation);
};
