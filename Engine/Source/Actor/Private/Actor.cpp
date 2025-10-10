#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Editor/Public/EditorEngine.h"
#include "Level/Public/Level.h"

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

	if (RootComponent)
	{
		RootComponent->Serialize(bInIsLoading, InOutHandle);
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
