#pragma once
#include "pch.h"
#include "Object.h"
#include "Actor.h"

class UExponentialHeightFogComponent;
class UBillboardComponent;
class UDecalComponent;
class UPrimitiveComponent;
class UPointLightComponent;
class UFXAAComponent;

class ULevel : public UObject
{
public:
	DECLARE_CLASS(ULevel, UObject)
	ULevel();
	~ULevel() override;

	void AddActor(AActor* InActor);
	void RemoveActor(AActor* InActor);
	void CollectComponentsToRender();

	// 타입별 컴포넌트 등록/해제 (상속 계층 자동 처리)
	void RegisterComponent(UActorComponent* Component);
	void UnregisterComponent(UActorComponent* Component);

	// 타입별 컴포넌트 리스트 조회 (참조 반환으로 성능 최적화)
	template<typename T>
	const TArray<T*>& GetComponentList() const;
	

	const TArray<AActor*>& GetActors() const;
	TArray<AActor*>& GetActors();
private:
	TArray<AActor*> Actors;

	// 타입별 컴포넌트 캐시 (상속 계층 포함)
	// Key: UClass*, Value: 해당 타입으로 캐스팅 가능한 모든 컴포넌트
	mutable std::unordered_map<const UClass*, TArray<UActorComponent*>> ComponentCache;

	// 타입별 컴포넌트 리스트 재구성
	template<typename T>
	void RebuildComponentCache() const;
};

// 템플릿 구현 (헤더에 포함)
template<typename T>
const TArray<T*>& ULevel::GetComponentList() const
{
	const UClass* TargetClass = T::StaticClass();

	// 캐시에 이미 있으면 바로 반환
	auto it = ComponentCache.find(TargetClass);
	if (it != ComponentCache.end())
	{
		return reinterpret_cast<const TArray<T*>&>(it->second);
	}

	// 없으면 재구성
	RebuildComponentCache<T>();
	return reinterpret_cast<const TArray<T*>&>(ComponentCache[TargetClass]);
}

template<typename T>
void ULevel::RebuildComponentCache() const
{
	const UClass* TargetClass = T::StaticClass();
	TArray<UActorComponent*>& Cache = ComponentCache[TargetClass];
	Cache.clear();

	// 모든 액터의 모든 컴포넌트를 순회하며 타입 체크
	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;

		for (UActorComponent* Comp : Actor->GetComponents())
		{
			if (!Comp) continue;

			// IsA를 통해 상속 계층 체크 (PointLight는 Light로도 조회 가능)
			if (Comp->IsA<T>())
			{
				Cache.Add(Comp);
			}
		}
	}
}