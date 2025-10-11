#include "pch.h"
#include "Component/Public/ActorComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

IMPLEMENT_CLASS(UActorComponent, UObject)

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
}

/**
 * @todo 현재는 계층 구조를 타고 내려오면서 재귀적으로 오브젝트들을 복사한다.
 *		 따라서, 계층의 중간에 놓인 오브젝트들을 Duplicate할 경우 위쪽 계층이 적절히
 *		 반영되지 않는다. 따라서, 이후에 플래그를 도입해서 이 문제를 해결한다.
 */
void UActorComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("Name"))
		{
			SetName(FName(InOutHandle["Name"].ToString()));
		}
	}
	else
	{
		InOutHandle["Name"] = GetName().ToString();
	}
}

UObject* UActorComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UActorComponent*>(Super::Duplicate(Parameters));

	DupObject->ComponentType = ComponentType;

	if (Owner != nullptr)
	{
		if (auto It = Parameters.DuplicationSeed.find(Owner); It != Parameters.DuplicationSeed.end())
		{
			DupObject->Owner = static_cast<AActor*>(It->second);
		}
		else
		{
			/** @todo 플래그를 도입해서 위쪽 계층이 반영될 수 있도록 변경한다. */
			UE_LOG_ERROR("Owner는 UActorComponent보다 먼저 생성되어야 합니다.");
			DupObject->Owner = nullptr;
		}
	}
	else
	{
		DupObject->Owner = nullptr;
	}

	return DupObject;
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent(float DeltaSeconds)
{

}

void UActorComponent::EndPlay()
{

}

/**
 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요 
 */
TObjectPtr<UClass> UActorComponent::GetSpecificWidgetClass() const
{
	return nullptr;
}
