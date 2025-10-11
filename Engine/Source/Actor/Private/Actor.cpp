#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Editor/Public/EditorEngine.h"
#include "Level/Public/Level.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	// to do: primitive factory로 빌보드 생성
	// BillBoardComponent = new UBillboardComponent(this, 5.0f);
	// OwnedComponents.push_back(TObjectPtr<UBillboardComponent>(BillBoardComponent));
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.clear();
}

void AActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// 0. 기존에 있던 모든 컴포넌트 자원을 할당 해제합니다. 
		for (UActorComponent* Component : OwnedComponents) { SafeDelete(Component); }
		OwnedComponents.clear();
		RootComponent = nullptr;

		// 1. 저장된 파일로부터 모든 컴포넌트를 불러와, 생성을 하고 등록합니다.
		TMap<FName, TObjectPtr<UActorComponent>> CreatedComponentsMap;
		if (InOutHandle.hasKey("Components"))
		{
			JSON ComponentsData = InOutHandle["Components"];
			for (auto& ComponentData : ComponentsData.ArrayRange())
			{
				std::string ClassName = ComponentData["ClassName"].ToString();
				FName ComponentName = FName(ComponentData["Name"].ToString());

				if (TObjectPtr<UClass> FoundClass = UClass::FindClass(FName(ClassName)))
				{
					if (TObjectPtr<UActorComponent> NewComponent = NewObject<UActorComponent>(this, FoundClass))
					{
						NewComponent->SetOwner(this);
						NewComponent->SetName(ComponentName);
						OwnedComponents.push_back(NewComponent);
						CreatedComponentsMap.emplace(ComponentName, NewComponent);
					}
				}
			}
		}
		
		// 2. 루트 컴포넌트를 찾아 루트 계층으로 설정합니다.
		if (InOutHandle.hasKey("RootComponentName"))
		{
			FName RootName = FName(InOutHandle["RootComponentName"].ToString());
			if (auto It = CreatedComponentsMap.find(RootName); It != CreatedComponentsMap.end())
			{
				RootComponent = Cast<USceneComponent>(It->second);
			}
		}

		// 3. 각 컴포넌트의 부모 컴포넌트를 설정하여, 최종적으로 액터의 계층 구조를 완성합니다.
		if (InOutHandle.hasKey("Components"))
		{
			JSON ComponentsData = InOutHandle["Components"];
			for (auto& ComponentData : ComponentsData.ArrayRange())
			{
				FName ComponentName = FName(ComponentData["Name"].ToString());
				if (auto It = CreatedComponentsMap.find(ComponentName); It != CreatedComponentsMap.end())
				{
					TObjectPtr<UActorComponent> Component = It->second;

					if (ComponentData.hasKey("ParentName"))
					{
						FName ParentName = FName(ComponentData["ParentName"].ToString());
						if (auto ParentIt = CreatedComponentsMap.find(ParentName); ParentIt != CreatedComponentsMap.end())
						{
							TObjectPtr<USceneComponent> ParentComponent = Cast<USceneComponent>(ParentIt->second);
							TObjectPtr<USceneComponent> ChildComponent = Cast<USceneComponent>(Component);
							if (ParentComponent && ChildComponent)
							{
								ChildComponent->SetParentAttachment(ParentComponent);
								ParentComponent->AddChild(ChildComponent);
							}
						}
					}

					// Now serialize component properties
					Component->Serialize(bInIsLoading, ComponentData);
				}
			}
		}
	}
	else
	{
		// 1. 루트 컴포넌트를 저장합니다.
		if (RootComponent)
		{
			InOutHandle["RootComponentName"] = RootComponent->GetName().ToString();
		}

		// 2. 하위 컴포넌트를 저장합니다.
		JSON ComponentsData = JSON::Make(JSON::Class::Array);
		for (const auto& Component : OwnedComponents)
		{
			if (Component)
			{
				JSON ComponentData;
				ComponentData["ClassName"] = Component->GetClass()->GetClassTypeName().ToString();
				Component->Serialize(bInIsLoading, ComponentData);
				ComponentsData.append(ComponentData);
			}
		}
		InOutHandle["Components"] = ComponentsData;
	}
}

UObject* AActor::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<AActor*>(Super::Duplicate(Parameters));

	/** @note 기본 생성자가 생성하는 컴포넌트들 맵에 업데이트 */
	if (RootComponent)
	{
		auto Params = InitStaticDuplicateObjectParams(RootComponent, DupObject, FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
		/** @todo CreatedObjects는 업데이트 안해도 괜찮은지 확인 필요 */
		Params.DuplicationSeed.emplace(RootComponent, DupObject->RootComponent);
		/** @note 값 채워넣기만 수행 */
		RootComponent->Duplicate(Params);
	}

	/** @todo 이후 다른 AActor를 상속받는 클래스들이 생성하는 컴포넌트를 어떻게 복제할 것인지 생각 */

	for (auto& Component : OwnedComponents)
	{
		/** 위에서 이미 처리함 */
		if (Component == RootComponent)
		{
			continue;
		}

		if (auto It = Parameters.DuplicationSeed.find(Component); It != Parameters.DuplicationSeed.end())
		{
			DupObject->OwnedComponents.emplace_back(static_cast<UActorComponent*>(It->second));
		}
		else
		{
			auto Params = InitStaticDuplicateObjectParams(Component, DupObject, FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupComponent = static_cast<UActorComponent*>(Component->Duplicate(Params));

			DupObject->OwnedComponents.emplace_back(DupComponent);
		}
	}

	return DupObject;
}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

void AActor::AddComponent(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent || !RootComponent)
	{
		return;
	}

	InComponent->SetOwner(this);
	OwnedComponents.push_back(InComponent);

	USceneComponent* InSceneComponent = Cast<USceneComponent>(InComponent);
	RootComponent->AddChild(InSceneComponent);
	InSceneComponent->SetParentAttachment(RootComponent);

	TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent);
	GEngine->GetCurrentLevel()->AddLevelPrimitiveComponent(PrimitiveComponent);
}

void AActor::Tick(float DeltaSeconds)
{
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->TickComponent(DeltaSeconds);
		}
	}

	static float TotalTime = 0.0f;
	static FVector OriginLocation = GetActorLocation();
	TotalTime += DeltaSeconds;
	// SetActorLocation(GetActorLocation() + FVector(0.05f * cosf(TotalTime * 0.01f), 0.05f * sinf(TotalTime * 0.01f), 0.0f));
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}
