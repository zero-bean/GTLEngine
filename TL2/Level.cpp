#include "pch.h"
#include "Level.h"
#include "DecalComponent.h"
#include "ExponentialHeightFogComponent.h"
#include "PrimitiveComponent.h"
#include "BillboardComponent.h"
#include "PointLightComponent.h"
#include "FXAAComponent.h"
#include "SpotLightComponent.h"

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

void ULevel::CollectComponentsToRender()
{
	DecalComponentList.clear();
	PrimitiveComponentList.clear();
	BillboardComponentList.clear();
	FogComponentList.clear();
	PointLightComponentList.clear();
	SpotLightComponentList.clear();
	FXAAComponentList.clear();

	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->GetActorHiddenInGame())
		{
			continue;
		}

		for (UActorComponent* ActorComponent : Actor->GetComponents())
		{
			if (UDecalComponent * DecalComponent = Cast<UDecalComponent>(ActorComponent))
			{
				DecalComponentList.Add(DecalComponent);
			}
			else if (UBillboardComponent* BillboardComponent = Cast<UBillboardComponent>(ActorComponent))
			{
				BillboardComponentList.Add(BillboardComponent);
			}
			else if (UExponentialHeightFogComponent* FogComponent = Cast<UExponentialHeightFogComponent>(ActorComponent))
			{
				if (FogComponent->IsRender())
				{
					FogComponentList.Add(FogComponent);
				}
			}
			else if (UFXAAComponent* FXAAComp = Cast<UFXAAComponent>(ActorComponent))
			{
				FXAAComponentList.Add(FXAAComp);
			}
			else if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(ActorComponent))
			{
				PointLightComponentList.Add(PointLightComponent);
			}
			else if (USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(ActorComponent))
			{
				SpotLightComponentList.Add(SpotLightComponent);
			}
			else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent))
			{
				PrimitiveComponentList.Add(PrimitiveComponent);
			}
		}
	}

	DecalComponentList.Sort([](const UDecalComponent* A, const UDecalComponent* B)
		{
			return A->GetSortOrder() < B->GetSortOrder();
		});
}

const TArray<AActor*>& ULevel::GetActors() const
{
	return Actors;
}

TArray<AActor*>& ULevel::GetActors() 
{
	return Actors;
}

template<>
TArray<UExponentialHeightFogComponent*>& ULevel::GetComponentList<UExponentialHeightFogComponent>()
{
	return FogComponentList;
}
template<>
TArray<UFXAAComponent*>& ULevel::GetComponentList<UFXAAComponent>()
{
	return FXAAComponentList;
}
template<>
TArray<UBillboardComponent*>& ULevel::GetComponentList<UBillboardComponent>()
{
	return BillboardComponentList;
}
template<>
TArray<UDecalComponent*>& ULevel::GetComponentList<UDecalComponent>()
{
	return DecalComponentList;
}
template<>
TArray<UPrimitiveComponent*>& ULevel::GetComponentList<UPrimitiveComponent>()
{
	return PrimitiveComponentList;
}
template<>
TArray<UPointLightComponent*>& ULevel::GetComponentList<UPointLightComponent>()
{
	return PointLightComponentList;
}
template<>
TArray<USpotLightComponent*>& ULevel::GetComponentList<USpotLightComponent>()
{
	return SpotLightComponentList;
}