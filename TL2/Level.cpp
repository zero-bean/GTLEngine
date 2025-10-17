#include "pch.h"
#include "Level.h"
#include "DecalComponent.h"
#include "ExponentialHeightFogComponent.h"
#include "PrimitiveComponent.h"
#include "BillboardComponent.h"
#include "PointLightComponent.h"
#include "FXAAComponent.h"

ULevel::ULevel()
{
}

ULevel::~ULevel()
{
	Actors.clear();
}

void ULevel::AddActor(AActor* InActor)
{
	if (InActor)	
	{
		if (InActor->GetClass()->Name == AActor::StaticClass()->Name)
		{
			InActor->InitEmptyActor();

		}
		Actors.Add(InActor);
	}
}

void ULevel::RemoveActor(AActor* InActor)
{
	if (InActor)
	{
		Actors.Remove(InActor);
		//delete InActor;
	}
}

void ULevel::RegisterComponent(UActorComponent* Component)
{
	if (!Component) return;

	// 캐시 무효화 (다음 GetComponentList 호출 시 재구성됨)
	ComponentCache.clear();
}

void ULevel::UnregisterComponent(UActorComponent* Component)
{
	if (!Component) return;

	// 캐시 무효화
	ComponentCache.clear();
}

void ULevel::CollectComponentsToRender()
{
	// 캐시 무효화 (레거시 호환성)
	ComponentCache.clear();
}

const TArray<AActor*>& ULevel::GetActors() const
{
	return Actors;
}

TArray<AActor*>& ULevel::GetActors()
{
	return Actors;
}