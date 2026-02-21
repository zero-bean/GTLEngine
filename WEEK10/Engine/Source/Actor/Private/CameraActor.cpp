#include "pch.h"
#include "Actor/Public/CameraActor.h"

IMPLEMENT_CLASS(ACameraActor, AActor)

ACameraActor::ACameraActor()
{
	// Enable tick for camera updates
	bCanEverTick = true;
}

ACameraActor::~ACameraActor() = default;

UClass* ACameraActor::GetDefaultRootComponent()
{
	return UCameraComponent::StaticClass();
}
