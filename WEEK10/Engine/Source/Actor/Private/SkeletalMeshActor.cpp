#include "pch.h"

#include "Actor/Public/SkeletalMeshActor.h"

IMPLEMENT_CLASS(ASkeletalMeshActor, AActor)

ASkeletalMeshActor::ASkeletalMeshActor()
	: SkeletalMeshComponent(nullptr)
{
}

UClass* ASkeletalMeshActor::GetDefaultRootComponent()
{
	return USkeletalMeshComponent::StaticClass();
}

USkeletalMeshComponent* ASkeletalMeshActor::GetSkeletalMeshComponent()
{
	return Cast<USkeletalMeshComponent>(GetRootComponent());
}
