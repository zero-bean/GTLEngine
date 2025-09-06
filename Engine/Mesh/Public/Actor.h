#pragma once

#include "Core/Public/Object.h"
#include "Mesh/Public/SceneComponent.h"
#include "pch.h"

class AActor : public UObject
{
	//UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
	
public:
	~AActor();

	void SetActorLocation(const FVector& Location);
	void SetActorRotation(const FVector& Rotation);
	void SetActorScale3D(const FVector& Scale);

	FVector& GetActorLocation();
	FVector& GetActorRotation();
	FVector& GetActorScale3D();

	//테스트용 코드
	//void Render(const URenderer& Renderer);

	template<typename T>
	T* CreateDefaultSubobject(const FString& Name)
	{
		T* NewComponent = new T();

		NewComponent->Outer = this;

		NewComponent->Name = Name;

		OwnedComponents.push_back(NewComponent);

		return NewComponent;
	}

	USceneComponent* RootComponent = nullptr;
	TArray<UActorComponent*> OwnedComponents;
private:
	
	

};


