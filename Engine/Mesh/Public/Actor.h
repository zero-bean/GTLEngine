#pragma once
#include "Core/Public/Object.h"
#include "Mesh/Public/SceneComponent.h"

/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()
class AActor : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	virtual ~AActor() override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FVector& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;

	template <typename T>
	T* CreateDefaultSubobject(const FString& InName);

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick();

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent; }
	TArray<UActorComponent*> GetOwnedComponents() const { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

private:
	USceneComponent* RootComponent = nullptr;
	TArray<UActorComponent*> OwnedComponents;
};

template <typename T>
T* AActor::CreateDefaultSubobject(const FString& InName)
{
	T* NewComponent = new T();

	///////////////////////////////////////////
	NewComponent->AddMemoryUsage(sizeof(T));
	///생성자에서 자신의 메모리 설정하게 수정 필요///
	NewComponent->SetOuter(this);
	//Outer 설정 시 Outer의 메모리 카운트에 자신의 메모리 합산 작업 수행

	////////////////////////////////////
	NewComponent->SetOwner(this);
	///RTTI로 Owner와 Outer 동일화 필////

	NewComponent->SetName(InName);
	OwnedComponents.push_back(NewComponent);

	return NewComponent;
}
