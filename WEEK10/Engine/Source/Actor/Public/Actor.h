#pragma once
#include "Core/Public/Object.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Core/Public/NewObject.h"
#include "Core/Public/Delegate.h"
#include "Core/Public/IDelegateProvider.h"
#include "Physics/Public/HitResult.h"

class UUUIDTextComponent;

// Actor-level overlap event signatures
DECLARE_DELEGATE(FActorBeginOverlapSignature,
	AActor*,              /* OverlappedActor */
	AActor*               /* OtherActor */
);

DECLARE_DELEGATE(FActorEndOverlapSignature,
	AActor*,              /* OverlappedActor */
	AActor*               /* OtherActor */
);

/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()

class AActor : public UObject, public IDelegateProvider
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	virtual ~AActor() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FQuaternion& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);
	virtual UClass* GetDefaultRootComponent();
	virtual void InitializeComponents();

	bool IsUniformScale() const;

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick(float DeltaTimes);

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent; }
	TArray<UActorComponent*>& GetOwnedComponents()  { return OwnedComponents; }

	/**
	 * @brief 특정 타입의 컴포넌트를 OwnedComponents에서 찾아 반환합니다.
	 * @tparam T 찾을 컴포넌트 타입
	 * @return 해당 타입의 컴포넌트를 찾으면 반환, 없으면 nullptr
	 */
	template<class T>
	T* GetComponentByClass() const
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "T는 UActorComponent를 상속받아야 합니다");

		for (UActorComponent* Component : OwnedComponents)
		{
			if (T* TypedComponent = Cast<T>(Component))
			{
				return TypedComponent;
			}
		}
		return nullptr;
	}

	/**
	 * @brief 특정 타입의 모든 컴포넌트를 OwnedComponents에서 찾아 반환합니다.
	 * @tparam T 찾을 컴포넌트 타입
	 * @return 해당 타입의 모든 컴포넌트를 담은 배열
	 */
	template<class T>
	TArray<T*> GetComponentsByClass() const
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "T는 UActorComponent를 상속받아야 합니다");

		TArray<T*> FoundComponents;
		for (UActorComponent* Component : OwnedComponents)
		{
			if (T* TypedComponent = Cast<T>(Component))
			{
				FoundComponents.Add(TypedComponent);
			}
		}
		return FoundComponents;
	}

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	// World Access
	class UWorld* GetWorld() const;

	const FVector& GetActorLocation() const;
	const FQuaternion& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	FVector GetActorForwardVector() const;
	FVector GetActorUpVector() const;
	FVector GetActorRightVector() const;

	/**
	 * @brief IDelegateProvider 구현 - Actor의 Delegate 목록 반환
	 * @return Lua에 노출할 Delegate 정보 배열
	 */
	TArray<FDelegateInfoBase*> GetDelegates() const override;

	template<class T>
	T* CreateDefaultSubobject(const FName& InName = FName::None)
	{
		static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

		// 2. NewObject를 호출할 때도 템플릿 타입 T를 사용하여 정확한 타입의 컴포넌트를 생성합니다.
		T* NewComponent = NewObject<T>(this);
		if (!InName.IsNone()) { NewComponent->SetName(InName); }

		// 3. 컴포넌트 생성이 성공했는지 확인하고 기본 설정을 합니다.
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.Add(NewComponent);
		}

		// 4. 정확한 타입(T*)으로 캐스팅 없이 바로 반환합니다.
		return NewComponent;
	}

	UActorComponent* CreateDefaultSubobject(UClass* Class);

	/**
	 * @brief 런타임에 이 액터에 새로운 컴포넌트를 생성하고 등록합니다.
	 * @tparam T UActorComponent를 상속받는 컴포넌트 타입
	 * @param InName 컴포넌트의 이름
	 * @return 생성된 컴포넌트를 가리키는 포인터
	 */
	template<class T>
	T* AddComponent()
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "추가할 클래스는 UActorComponent를 반드시 상속 받아야 합니다");

		T* NewComponent = NewObject<T>(this);

		if (NewComponent)
		{
			RegisterComponent(NewComponent);
		}

		return NewComponent;
	}
	UActorComponent* AddComponent(UClass* InClass);

	void RegisterComponent(UActorComponent* InNewComponent);

	bool RemoveComponent(UActorComponent* InComponentToDelete, bool bShouldDetachChildren = false);

	bool CanTick() const { return bCanEverTick; }
	void SetCanTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

	bool CanTickInEditor() const { return bTickInEditor; }
	void SetTickInEditor(bool InbTickInEditor) { bTickInEditor = InbTickInEditor; }

	bool IsPendingDestroy() const { return bIsPendingDestroy; }
	void SetIsPendingDestroy(bool bInIsPendingDestroy) { bIsPendingDestroy = bInIsPendingDestroy; }

	bool IsTemplate() const { return bIsTemplate; }
	void SetIsTemplate(bool bInIsTemplate);

protected:
	bool bCanEverTick = false;
	bool bTickInEditor = false;
	bool bBegunPlay = false;
	/** @brief True if the actor is marked for destruction. */
	bool bIsPendingDestroy = false;
	/** @brief True if the actor is a template. Hides in PIE but shows in Editor. */
	bool bIsTemplate = false;

private:
	USceneComponent* RootComponent = nullptr;
	TArray<UActorComponent*> OwnedComponents;

public:
	virtual UObject* Duplicate() override;
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

	virtual UObject* DuplicateForEditor() override;
	virtual void DuplicateSubObjectsForEditor(UObject* DuplicatedObject) override;

	/**
	 * @brief 템플릿 액터로부터 인스턴스를 생성하고 레벨에 배치합니다.
	 * @details bIsTemplate을 false로 설정하고, BeginPlay 이전에 Location/Rotation을 설정합니다.
	 *          자동으로 지정된 Level(또는 템플릿의 Outer Level)에 추가되어 렌더링 및 Tick이 작동합니다.
	 * @param TargetLevel 인스턴스를 배치할 대상 Level (nullptr이면 템플릿의 Outer Level 사용)
	 * @param InLocation 인스턴스의 초기 위치
	 * @param InRotation 인스턴스의 초기 회전
	 * @return 복제된 액터 (템플릿이 아닌 일반 액터, Level에 추가된 상태)
	 */
	AActor* DuplicateFromTemplate(
		class ULevel* TargetLevel = nullptr,
		const FVector& InLocation = FVector::Zero(),
		const FQuaternion& InRotation = FQuaternion::Identity()
	);
// Collision Section
public:
	ECollisionTag GetCollisionTag() const { return CollisionTag; }
	void SetCollisionTag(const ECollisionTag Tag) { CollisionTag = Tag; }

	// === Overlap Query API ===
	bool IsOverlappingActor(const AActor* OtherActor) const;
	void GetOverlappingComponents(const AActor* OtherActor, TArray<UPrimitiveComponent*>& OutComponents) const;

	// === Collision Event Delegates ===
	// Public so users can bind to these events
	FActorBeginOverlapSignature OnActorBeginOverlap;
	FActorEndOverlapSignature OnActorEndOverlap;

private:
	ECollisionTag CollisionTag = ECollisionTag::None;
};
