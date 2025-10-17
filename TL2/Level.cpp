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
	ComponentCache.clear();
	Actors.clear();
}

void ULevel::AddActor(AActor* InActor)
{
	if (InActor)
	{
		// AActor 클래스인 경우 (정확히 AActor, 파생 클래스 제외)
		if (std::strcmp(InActor->GetClass()->Name, "AActor") == 0)
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
		// 캐시 무효화 (액터가 제거되면 해당 액터의 컴포넌트도 사라짐)
		ComponentCache.clear();
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

void ULevel::ClearComponentCache()
{
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