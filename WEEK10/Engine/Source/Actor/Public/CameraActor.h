#pragma once
#include "Actor/Public/Actor.h"
#include "Component/Public/CameraComponent.h"

/**
 * ACameraActor
 * Placeable camera actor in the level
 * Similar to Unreal Engine's ACameraActor
 */
UCLASS()
class ACameraActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACameraActor, AActor)

public:
	ACameraActor();
	virtual ~ACameraActor() override;

	virtual UClass* GetDefaultRootComponent() override;

	/**
	 * Get the camera component
	 */
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	UCameraComponent* CameraComponent = nullptr;
};
