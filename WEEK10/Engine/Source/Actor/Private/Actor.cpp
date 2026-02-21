#include "pch.h"
#include "Actor/Public/Actor.h"

#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Public/EditorIconComponent.h"
#include "Component/Public/LightComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Component/Public/ScriptComponent.h"  // FDelegateInfo 템플릿 구현용
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
}

AActor::AActor(UObject* InOuter) : AActor()
{
	SetOuter(InOuter);
}

TArray<FDelegateInfoBase*> AActor::GetDelegates() const
{
	TArray<FDelegateInfoBase*> Result;

	// const_cast 필요 (Delegate는 mutable)
	AActor* MutableThis = const_cast<AActor*>(this);

	Result.Add(MakeDelegateInfo("OnActorBeginOverlap", &MutableThis->OnActorBeginOverlap));
	Result.Add(MakeDelegateInfo("OnActorEndOverlap", &MutableThis->OnActorEndOverlap));

	return Result;
}

UActorComponent* AActor::CreateDefaultSubobject(UClass* Class)
{
	UActorComponent* NewComponent = Cast<UActorComponent>(NewObject(Class, this));
	if (NewComponent)
	{
		NewComponent->SetOwner(this);
		OwnedComponents.Add(NewComponent);
	}

	return NewComponent;
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.Empty();
}

void AActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // 불러오기 (Load)
    if (bInIsLoading)
    {
    	// Name 로드 (NewObject가 자동으로 붙인 suffix를 덮어씀)
    	FString NameString;
    	if (FJsonSerializer::ReadString(InOutHandle, "Name", NameString))
    	{
    		SetName(NameString);
    	}
    	// 컴포넌트 포인터와 JSON 데이터를 임시 저장할 구조체
        struct FSceneCompData
        {
            USceneComponent* Component = nullptr;
            FString ParentName; // ParentName을 임시 저장
        };

        JSON ComponentsJson;
        if (FJsonSerializer::ReadArray(InOutHandle, "Components", ComponentsJson))
        {
			TMap<FString, FSceneCompData> ComponentMap;
	        TArray<FSceneCompData*> LoadList;
	        // --- [PASS 1: Component Creation & Data Load] ---
            for (JSON& ComponentData : ComponentsJson.ArrayRange())
            {
                FString TypeString;
                FString NameString;

                FJsonSerializer::ReadString(ComponentData, "Type", TypeString);
                FJsonSerializer::ReadString(ComponentData, "Name", NameString);

            	UClass* ComponentClass = UClass::FindClass(TypeString);
                UActorComponent* NewComp = Cast<UActorComponent>(NewObject(ComponentClass, this));
            	NewComp->SetName(NameString);

                if (NewComp)
                {
                	NewComp->SetOwner(this);
                	OwnedComponents.Add(NewComp);
                    NewComp->Serialize(bInIsLoading, ComponentData);

                	if (USceneComponent* NewSceneComp = Cast<USceneComponent>(NewComp))
                	{
                		FString ParentNameStd;
                		FJsonSerializer::ReadString(ComponentData, "ParentName", ParentNameStd, ""); // 부모 이름 로드

                		FSceneCompData LoadData;
                		LoadData.Component = NewSceneComp;
                		LoadData.ParentName = ParentNameStd;

                		ComponentMap[NameString] = LoadData;
                		LoadList.Add(&ComponentMap[NameString]);
                	}
                }
            }

            // --- [PASS 2: Hierarchy Rebuild] ---
            for (FSceneCompData* LoadDataPtr : LoadList)
            {
                USceneComponent* ChildComp = LoadDataPtr->Component;
                const FString& ParentName = LoadDataPtr->ParentName;

                if (!ParentName.empty())
                {
                	auto* ParentInfoPtr = ComponentMap.Find(ParentName);

                	if (ParentInfoPtr)
                	{
                		USceneComponent* ParentComp = ParentInfoPtr->Component;

                		// 부착 함수 호출
                		if (ParentComp)
                		{
                			ChildComp->AttachToComponent(ParentComp, true);
                		}
                	}
                    else
                    {
                        UE_LOG("Failed to find parent component: %s", ParentName.c_str());
                    }
                }
                // ParentName이 비어있으면 루트 컴포넌트
                else
                {
                	SetRootComponent(ChildComp);
                }
            }

        	// Icon 바인딩을 위한 컴포넌트 수집 (순회 중 배열 수정 방지)
        	TArray<ULightComponent*> LightComponents;
        	TArray<UDecalComponent*> DecalComponents;

        	for (UActorComponent* Component : OwnedComponents)
        	{
        		if (ULightComponent* LightComponent = Cast<ULightComponent>(Component))
        		{
        			LightComponents.Add(LightComponent);
        		}
        		else if (UDecalComponent* DecalComponent = Cast<UDecalComponent>(Component))
        		{
        			DecalComponents.Add(DecalComponent);
        		}
        	}

        	// Icon 바인딩 수행 (AddComponent 호출로 인한 OwnedComponents 수정 가능)
        	for (ULightComponent* LightComponent : LightComponents)
        	{
        		LightComponent->RefreshVisualizationIconBinding();
        	}

        	for (UDecalComponent* DecalComponent : DecalComponents)
        	{
        		DecalComponent->RefreshVisualizationIconBinding();
        	}

        	if (RootComponent)
        	{
    			FVector Location, RotationEuler, Scale;

    		    FJsonSerializer::ReadVector(InOutHandle, "Location", Location, GetActorLocation());
    		    FJsonSerializer::ReadVector(InOutHandle, "Rotation", RotationEuler, GetActorRotation().ToEuler());
    		    FJsonSerializer::ReadVector(InOutHandle, "Scale", Scale, GetActorScale3D());

    		    SetActorLocation(Location);
    		    SetActorRotation(FQuaternion::FromEuler(RotationEuler));
        		SetActorScale3D(Scale);
        	}

			FString bCanEverTickString;
			FJsonSerializer::ReadString(InOutHandle, "bCanEverTick", bCanEverTickString, "false");
			bCanEverTick = bCanEverTickString == "true" ? true : false;

			FString bTickInEditorString;
			FJsonSerializer::ReadString(InOutHandle, "bTickInEditor", bTickInEditorString, "false");
			bTickInEditor = bTickInEditorString == "true" ? true : false;

			FString bIsTemplateString;
			FJsonSerializer::ReadString(InOutHandle, "bIsTemplate", bIsTemplateString, "false");
			bIsTemplate = bIsTemplateString == "true" ? true : false;

        	uint32 CollisionTagInt;
			FJsonSerializer::ReadUint32(InOutHandle, "CollisionTag", CollisionTagInt, 0);
			CollisionTag = static_cast<ECollisionTag>(CollisionTagInt);
        }
    }
    // 저장 (Save)
    else
    {
    	InOutHandle["Name"] = GetName().ToString();

		InOutHandle["Location"] = FJsonSerializer::VectorToJson(GetActorLocation());
        InOutHandle["Rotation"] = FJsonSerializer::VectorToJson(GetActorRotation().ToEuler());
        InOutHandle["Scale"] = FJsonSerializer::VectorToJson(GetActorScale3D());

		InOutHandle["bCanEverTick"] = bCanEverTick ? "true" : "false";
		InOutHandle["bTickInEditor"] = bTickInEditor ? "true" : "false";
		InOutHandle["bIsTemplate"] = bIsTemplate ? "true" : "false";
		InOutHandle["CollisionTag"] = static_cast<uint32>(CollisionTag);

        JSON ComponentsJson = json::Array();

        for (UActorComponent* Component : OwnedComponents)
        {
        	// Visualization Icon 컴포넌트는 저장하지 않음
        	if (UEditorIconComponent* IconComp = Cast<UEditorIconComponent>(Component))
        	{
        		if (IconComp->IsVisualizationComponent())
        		{
        			continue; // Visualization Icon은 스킵
        		}
        	}

        	JSON ComponentJson;
        	ComponentJson["Type"] = Component->GetClass()->GetName().ToString();
        	ComponentJson["Name"] = Component->GetName().ToString();

	        if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
        	{
        		USceneComponent* Parent = SceneComponent->GetAttachParent();
        		FString ParentName = Parent ? Parent->GetName().ToString() : "";
        		ComponentJson["ParentName"] = ParentName;
        	}

        	Component->Serialize(bInIsLoading, ComponentJson);

        	// RootComponent의 경우 Location은 Actor에 저장되므로 ZeroVector로 오버라이드
        	if (Component == RootComponent)
        	{
        		ComponentJson["Location"] = FJsonSerializer::VectorToJson(FVector::ZeroVector());
        	}

            ComponentsJson.append(ComponentJson);
        }
        InOutHandle["Components"] = ComponentsJson;
    }
}


void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FQuaternion& InRotation) const
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

UClass* AActor::GetDefaultRootComponent()
{
	return USceneComponent::StaticClass();
}

void AActor::InitializeComponents()
{
	USceneComponent* SceneComp = Cast<USceneComponent>(CreateDefaultSubobject(GetDefaultRootComponent()));
	SetRootComponent(SceneComp);

	if (SceneComp->IsA(ULightComponent::StaticClass()))
	{
		static_cast<ULightComponent*>(SceneComp)->EnsureVisualizationIcon();
	}
	else if (SceneComp->IsExactly(USceneComponent::StaticClass()))
	{
		// 빈 Actor인 경우 Actor 아이콘 추가
		UEditorIconComponent* ActorIcon = CreateDefaultSubobject<UEditorIconComponent>();
		ActorIcon->AttachToComponent(SceneComp);
		ActorIcon->SetIsVisualizationComponent(true);
		ActorIcon->SetSprite(UAssetManager::GetInstance().LoadTexture("Asset/Icon/Actor_64x.png"));
		ActorIcon->SetRelativeScale3D(FVector(2.f, 2.f, 2.f));
		ActorIcon->SetScreenSizeScaled(true);
	}

	UUUIDTextComponent* UUID = CreateDefaultSubobject<UUUIDTextComponent>();
	UUID->AttachToComponent(GetRootComponent());
	UUID->SetOffset(5.0f);
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

const FQuaternion& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

FVector AActor::GetActorForwardVector() const
{
	assert(RootComponent);
	return RootComponent->GetForwardVector();
}

FVector AActor::GetActorUpVector() const
{
	assert(RootComponent);
	return RootComponent->GetUpVector();
}

FVector AActor::GetActorRightVector() const
{
	assert(RootComponent);
	return RootComponent->GetRightVector();
}

UActorComponent* AActor::AddComponent(UClass* InClass)
{
	if (!InClass->IsChildOf(UActorComponent::StaticClass())) { return nullptr; }
	UActorComponent* NewComponent = Cast<UActorComponent>(NewObject(InClass, this));

	if (NewComponent)
	{
		RegisterComponent(NewComponent);
	}

	NewComponent->BeginPlay();

	// LightComponent인 경우 에디터 아이콘 생성
	if (ULightComponent* LightComp = Cast<ULightComponent>(NewComponent))
	{
		LightComp->EnsureVisualizationIcon();
	}


	return NewComponent;
}

void AActor::RegisterComponent(UActorComponent* InNewComponent)
{
	if (!InNewComponent)
	{
		return;
	}

	if (InNewComponent->GetOwner() != this)
	{
		InNewComponent->SetOwner(this);
	}

	// 중복 추가 방지
	auto It = std::find(OwnedComponents.begin(), OwnedComponents.end(), InNewComponent);
	if (It == OwnedComponents.end())
	{
		OwnedComponents.Add(InNewComponent);
	}

	// FIX: 액터가 속한 Level에 등록 (GWorld 대신 GetOuter 사용)
	// 이렇게 하면 Preview World의 액터는 Preview World의 Level에 등록됨
	ULevel* OwningLevel = Cast<ULevel>(GetOuter());
	if (OwningLevel)
	{
		OwningLevel->RegisterComponent(InNewComponent);
	}
}

bool AActor::RemoveComponent(UActorComponent* InComponentToDelete, bool bShouldDetachChildren)
{
    if (!InComponentToDelete) { return false; }

	if (!OwnedComponents.Contains(InComponentToDelete)) { return false; }

    if (InComponentToDelete == RootComponent)
    {
    	UE_LOG_WARNING("루트 컴포넌트는 제거할 수 없습니다.");
        return false;
    }

	if (ULightComponent* LightComponent = Cast<ULightComponent>(InComponentToDelete))
	{
		// 라이트에 연결된 시각화 아이콘이 있으면, 자식 분리 옵션과 무관하게 먼저 삭제
		if (UEditorIconComponent* Icon = LightComponent->GetEditorIconComponent())
		{
			// 아이콘은 반드시 파괴되도록 자식 분리 옵션을 false로 호출
			RemoveComponent(Icon, false);
			LightComponent->SetEditorIconComponent(nullptr);
		}

		// 라이트 컴포넌트 자체를 등록 해제
		// FIX: Actor가 속한 Level에서 Unregister (GWorld 대신 GetOuter 사용)
		if (ULevel* OwningLevel = Cast<ULevel>(GetOuter()))
		{
			OwningLevel->UnregisterComponent(LightComponent);
		}
	}

	if (UDecalComponent* DecalComponent = Cast<UDecalComponent>(InComponentToDelete))
	{
		// 데칼에 연결된 시각화 아이콘이 있으면, 자식 분리 옵션과 무관하게 먼저 삭제
		if (UEditorIconComponent* Icon = DecalComponent->GetEditorIconComponent())
		{
			// 아이콘은 반드시 파괴되도록 자식 분리 옵션을 false로 호출
			RemoveComponent(Icon, false);
			DecalComponent->SetEditorIconComponent(nullptr);
		}
	}
    if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InComponentToDelete))
    {
		// FIX: Actor가 속한 Level에서 Unregister (GWorld 대신 GetOuter 사용)
		if (ULevel* OwningLevel = Cast<ULevel>(GetOuter()))
		{
			OwningLevel->UnregisterComponent(PrimitiveComponent);
		}
    }

    if (USceneComponent* SceneComponent = Cast<USceneComponent>(InComponentToDelete))
    {
    	USceneComponent* Parent = SceneComponent->GetAttachParent();
        SceneComponent->DetachFromComponent();

        TArray<USceneComponent*> ChildrenToProcess = SceneComponent->GetChildren();
        if (bShouldDetachChildren)
        {
            for (USceneComponent* Child : ChildrenToProcess)
            {
                Child->DetachFromComponent();
            	Child->AttachToComponent(Parent);
            }
        }
        else
        {
            // 자식을 함께 파괴함 (Destroy - 런타임 기본 방식)
            for (USceneComponent* Child : ChildrenToProcess)
            {
                RemoveComponent(Child);
            }
        }
    }

	if (GEditor->GetEditorModule()->GetSelectedComponent() == InComponentToDelete)
	{
		GEditor->GetEditorModule()->SelectComponent(nullptr);
	}
	OwnedComponents.Remove(InComponentToDelete);
    SafeDelete(InComponentToDelete);
    return true;
}

UObject* AActor::Duplicate()
{
	AActor* Actor = Cast<AActor>(Super::Duplicate());
	Actor->bCanEverTick = bCanEverTick;
	Actor->bIsTemplate = bIsTemplate;
	Actor->SetName(GetName());  // Name 복사
	Actor->CollisionTag = CollisionTag;
	return Actor;
}

void AActor::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	AActor* DuplicatedActor = Cast<AActor>(DuplicatedObject);

	// { 복제 전 Component, 복제 후 Component }
	TMap<UActorComponent*, UActorComponent*> OldToNewComponentMap;

	// EditorOnly가 아닌 모든 컴포넌트를 복제해 맵에 저장
	for (UActorComponent* OldComponent : OwnedComponents)
	{
		if (OldComponent && !OldComponent->IsEditorOnly())
		{
			UActorComponent* NewComponent = Cast<UActorComponent>(OldComponent->Duplicate());
			NewComponent->SetOwner(DuplicatedActor);
			DuplicatedActor->OwnedComponents.Add(NewComponent);
			OldToNewComponentMap[OldComponent] = NewComponent;
		}
	}

	// 복제된 컴포넌트들 계층 구조 재조립
	for (auto const& [OldComp, NewComp] : OldToNewComponentMap)
	{
		USceneComponent* OldSceneComp = Cast<USceneComponent>(OldComp);
		if (!OldSceneComp) { continue; } // SceneComponent Check
		USceneComponent* NewSceneComp = Cast<USceneComponent>(NewComp);
		USceneComponent* OldParent = OldSceneComp->GetAttachParent();

		// 원본 부모가 있었다면, 그에 맞는 새 부모를 찾아 연결
		while (OldParent)
		{
			UActorComponent** FoundNewParentValuePtr = OldToNewComponentMap.Find(OldParent);
			if (FoundNewParentValuePtr)
			{
				NewSceneComp->AttachToComponent(Cast<USceneComponent>(*FoundNewParentValuePtr));
				break;
			}

			// 부모가 EditorOnly라 맵에 없다면, 조부모를 찾아 다시 시도
			OldParent = OldParent->GetAttachParent();
		}
	}

	// Set Root Component
	USceneComponent* OldRoot = GetRootComponent();

	if (OldRoot)
	{
		UActorComponent** NewComponentPtr = OldToNewComponentMap.Find(OldRoot);

		if (NewComponentPtr)
		{
			UActorComponent* NewComp = *NewComponentPtr;
			USceneComponent* NewRoot = Cast<USceneComponent>(NewComp);

			if (NewRoot)
			{
				DuplicatedActor->SetRootComponent(NewRoot);
			}
		}
	}
}

UObject* AActor::DuplicateForEditor()
{
	AActor* Actor = Cast<AActor>(NewObject(GetClass()));
	Actor->bCanEverTick = bCanEverTick;
	Actor->bIsTemplate = bIsTemplate;
	// Name은 복사하지 않음 - NewObject가 자동으로 suffix를 붙여 고유한 이름 생성
	DuplicateSubObjectsForEditor(Actor);
	return Actor;
}

void AActor::DuplicateSubObjectsForEditor(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	AActor* DuplicatedActor = Cast<AActor>(DuplicatedObject);

	// { 복제 전 Component, 복제 후 Component }
	TMap<UActorComponent*, UActorComponent*> OldToNewComponentMap;

	// EditorOnly 체크 없이 모든 컴포넌트를 복제해 맵에 저장
	// Visualization 컴포넌트는 제외 (EnsureVisualizationIcon으로 나중에 생성)
	for (UActorComponent* OldComponent : OwnedComponents)
	{
		if (OldComponent)
		{
			if (UEditorIconComponent* IconComp = Cast<UEditorIconComponent>(OldComponent))
			{
				if (IconComp->IsVisualizationComponent())
				{
					continue;
				}
			}

			UActorComponent* NewComponent = Cast<UActorComponent>(OldComponent->Duplicate());
			NewComponent->SetOwner(DuplicatedActor);
			DuplicatedActor->OwnedComponents.Add(NewComponent);
			OldToNewComponentMap[OldComponent] = NewComponent;
		}
	}

	// 복제된 컴포넌트들 계층 구조 재조립
	for (auto const& [OldComp, NewComp] : OldToNewComponentMap)
	{
		USceneComponent* OldSceneComp = Cast<USceneComponent>(OldComp);
		if (!OldSceneComp)
		{
			continue;
		}
		USceneComponent* NewSceneComp = Cast<USceneComponent>(NewComp);
		USceneComponent* OldParent = OldSceneComp->GetAttachParent();

		// 원본 부모가 있었다면, 그에 맞는 새 부모를 찾아 연결
		if (OldParent)
		{
			UActorComponent** FoundNewParentValuePtr = OldToNewComponentMap.Find(OldParent);
			if (FoundNewParentValuePtr)
			{
				NewSceneComp->AttachToComponent(Cast<USceneComponent>(*FoundNewParentValuePtr));
			}
		}
	}

	// Set Root Component
	USceneComponent* OldRoot = GetRootComponent();
	if (OldRoot)
	{
		UActorComponent** FoundNewRootValuePtr = OldToNewComponentMap.Find(OldRoot);

		if (FoundNewRootValuePtr)
		{
			DuplicatedActor->SetRootComponent(Cast<USceneComponent>(*FoundNewRootValuePtr));
		}
	}
}

UWorld* AActor::GetWorld() const
{
	TObjectPtr<ULevel> Level = Cast<ULevel>(GetOuter());
	if (Level)
	{
		return Level->GetOwningWorld();
	}
	return nullptr;
}

void AActor::Tick(float DeltaTimes)
{
	// PIE World에서 입력 차단 중이면 Tick 스킵 (Shift + F1 detach)
	UWorld* World = GetWorld();
	if (World && World->IsIgnoringInput())
	{
		return;
	}

	for (auto& Component : OwnedComponents)
	{
		if (Component && Component->CanEverTick())
		{
			Component->TickComponent(DeltaTimes);
		}
	}
}

void AActor::BeginPlay()
{
	if (bBegunPlay) return;
	bBegunPlay = true;

	// BeginPlay 중 AddComponent 호출로 인한 배열 수정 방지를 위해 복사본 사용
	TArray<UActorComponent*> ComponentsCopy = OwnedComponents;
	for (auto& Component : ComponentsCopy)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}
}

void AActor::EndPlay()
{
	if (!bBegunPlay) return;
	bBegunPlay = false;
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->EndPlay();
		}
	}
}

// === Overlap Query Implementation ===

bool AActor::IsOverlappingActor(const AActor* OtherActor) const
{
	if (!OtherActor)
		return false;

	// Iterate through all owned primitive components
	for (UActorComponent* Component : OwnedComponents)
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
		{
			// Check overlap cache
			if (PrimComp->GetOverlapInfos().Num() > 0 &&
			    PrimComp->IsOverlappingActor(OtherActor))
			{
				return true;  // Found at least one overlap
			}
		}
	}

	return false;
}

void AActor::GetOverlappingComponents(const AActor* OtherActor, TArray<UPrimitiveComponent*>& OutComponents) const
{
	if (!OtherActor)
		return;

	for (UActorComponent* Component : OwnedComponents)
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
		{
			if (PrimComp->IsOverlappingActor(OtherActor))
			{
				OutComponents.Add(PrimComp);
			}
		}
	}
}

AActor* AActor::DuplicateFromTemplate(ULevel* TargetLevel, const FVector& InLocation, const FQuaternion& InRotation)
{
	// 일반 Duplicate 수행
	AActor* NewActor = Cast<AActor>(Duplicate());

	if (NewActor)
	{
		// 템플릿 플래그 해제
		NewActor->SetIsTemplate(false);

		// TODO: 리팩토링 필요
		// - Level 내부 메소드를 직접 호출하지 말고 통합된 등록 함수로 위임
		// - World의 bBegunPlay 체크 없이 직접 BeginPlay() 호출 (일관성 문제)
		// - 제안: ULevel::RegisterActor(AActor*, bool bCallBeginPlay) 같은 통합 메소드

		// Level에 추가
		ULevel* LevelToAddTo = TargetLevel;
		if (!LevelToAddTo)
		{
			// TargetLevel이 지정되지 않으면 현재 활성 World의 Level 사용 (PIE World 대응)
			if (GWorld)
			{
				LevelToAddTo = GWorld->GetLevel();
			}
			else
			{
				// Fallback: 템플릿의 Outer Level (Editor World)
				LevelToAddTo = Cast<ULevel>(GetOuter());
			}
		}

		if (LevelToAddTo)
		{
			// Outer 설정
			NewActor->SetOuter(LevelToAddTo);

			// BeginPlay 이전에 Location/Rotation 설정 (스크립트에서 사용 가능)
			NewActor->SetActorLocation(InLocation);
			NewActor->SetActorRotation(InRotation);

			// Level의 Actor 리스트에 추가
			LevelToAddTo->AddActorToLevel(NewActor);

			// BeginPlay 호출 (이 시점에 Location/Rotation이 이미 설정되어 있음)
			NewActor->BeginPlay();

			// Level의 Component 시스템에 등록
			LevelToAddTo->AddLevelComponent(NewActor);
		}
		else
		{
			UE_LOG_ERROR("AActor::DuplicateFromTemplate - Level을 찾을 수 없습니다. 인스턴스가 레벨에 추가되지 않았습니다.");
		}
	}

	return NewActor;
}

void AActor::SetIsTemplate(bool bInIsTemplate)
{
	// 값이 변경되지 않으면 아무것도 하지 않음
	if (bIsTemplate == bInIsTemplate)
	{
		return;
	}

	bool bWasTemplate = bIsTemplate;
	bIsTemplate = bInIsTemplate;

	// Level 캐시 업데이트
	if (ULevel* Level = Cast<ULevel>(GetOuter()))
	{
		if (bIsTemplate)
		{
			// false -> true: 캐시에 추가
			Level->RegisterTemplateActor(this);
		}
		else if (bWasTemplate)
		{
			// true -> false: 캐시에서 제거
			Level->UnregisterTemplateActor(this);
		}
	}
}
