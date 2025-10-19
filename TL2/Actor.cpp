#include "pch.h"
#include "Actor.h"
#include "SceneComponent.h"
#include "ObjectFactory.h"
#include "ShapeComponent.h"
#include "MeshComponent.h"
#include "BillboardComponent.h"
#include "TextRenderComponent.h"
#include "MovementComponent.h"
#include "World.h"
#include "Level.h"

AActor::AActor()
{
    Name = "DefaultActor";
    //필요한 것만 Default Component를 생성해주도록 바꾸고 AActor만 생성하는 경우 Level에 등록할때 생성하도록 바꿈
}

AActor::~AActor()
{
    for (UActorComponent* Comp : OwnedComponents)
    {
        if (Comp)
        {
            ObjectFactory::DeleteObject(Comp);
        }
    }
    OwnedComponents.Empty();
}

void AActor::InitEmptyActor()
{
    if (!RootComponent)
    {
        USceneComponent* DefaultComponent = CreateDefaultSubobject<USceneComponent>(FName("DefaultSceneComponent"));
        RootComponent = DefaultComponent;

        SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName("SpriteComponent"));
        if (SpriteComponent)
        {
            SpriteComponent->SetTexture(FString("Editor/Icon/EmptyActor.dds"));
            SpriteComponent->SetRelativeLocation(RootComponent->GetWorldLocation());
            SpriteComponent->SetEditable(false);
            SpriteComponent->SetupAttachment(RootComponent);
        }
    }  
}

void AActor::BeginPlay()
{
    // Activate all owned components
    for (UActorComponent* Component : OwnedComponents)
    {
        if (Component && Component->IsActive())
        {
            Component->BeginPlay();
        }
    }
}

void AActor::Tick(float DeltaSeconds)
{
    // 🔹 현재 활성 월드 타입 가져오기
    EWorldType CurrentWorldType = EWorldType::None;
    if (GEngine)
    {
        if (UWorld* World = GEngine->GetActiveWorld())
        {
            CurrentWorldType = World->WorldType;
        }
    }

    // 🔹 소유한 컴포넌트들 Tick
    for (UActorComponent* Component : OwnedComponents)
    {
        if (!Component || !Component->IsActive() || !Component->CanEverTick())
            continue;

        // ✅ WorldTickMode 검사
        const EComponentWorldTickMode TickMode = Component->WorldTickMode; // 게터 있으면
        bool bShouldTick = false;

        switch (TickMode)
        {
        case EComponentWorldTickMode::All:
            bShouldTick = true;
            break;
        case EComponentWorldTickMode::PIEOnly:
            bShouldTick = (CurrentWorldType == EWorldType::PIE);
            break;
        case EComponentWorldTickMode::GameOnly:
            bShouldTick = (CurrentWorldType == EWorldType::Game);
            break;
        case EComponentWorldTickMode::EditorOnly:
            bShouldTick = (CurrentWorldType == EWorldType::Editor);
            break;
        default:
            break;
        }

        // ❌ 틱 조건 불충족 시 패스
        if (!bShouldTick)
            continue;

        // ✅ 틱 수행
        Component->TickComponent(DeltaSeconds);
    }
}

/**
 * @brief Endplay 전파 함수
 * @param EndPlayReason Endplay 이유, Type에 따른 다른 설정이 가능함
 */
void AActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (UActorComponent* Component : OwnedComponents)
    {
        Component->EndPlay(EndPlayReason);
    }
}

void AActor::Destroy()
{
    if (!bCanEverTick) return;
    // Prefer world-managed destruction to remove from world actor list
    if (GetWorld())
    {
        // Avoid using 'this' after the call
        GetWorld()->DestroyActor(this);
        return;
    }
    // Fallback: directly delete the actor via factory
    ObjectFactory::DeleteObject(this);
}

// ───────────────
// Transform API
// ───────────────
void AActor::SetActorTransform(const FTransform& InNewTransform) const
{
    if (RootComponent)
    {
        RootComponent->SetWorldTransform(InNewTransform);
    }
}


FTransform AActor::GetActorTransform() const
{
    return RootComponent ? RootComponent->GetWorldTransform() : FTransform();
}

void AActor::SetActorLocation(const FVector& InNewLocation)
{
    if (RootComponent)
    {
        RootComponent->SetWorldLocation(InNewLocation);
    }
}

FVector AActor::GetActorLocation() const
{
    return RootComponent ? RootComponent->GetWorldLocation() : FVector();
}

void AActor::SetActorRotation(const FVector& InEulerDegree) const
{
    if (RootComponent)
    {
        RootComponent->SetWorldRotation(FQuat::MakeFromEuler(InEulerDegree));
    }
}

void AActor::SetActorRotation(const FQuat& InQuat) const
{
    if (RootComponent)
    {
        RootComponent->SetWorldRotation(InQuat);
    }
}

FQuat AActor::GetActorRotation() const
{
    return RootComponent ? RootComponent->GetWorldRotation() : FQuat();
}

void AActor::SetActorScale(const FVector& InNewScale) const
{
    if (RootComponent)
    {
        RootComponent->SetWorldScale(InNewScale);
    }
}

FVector AActor::GetActorScale() const
{
    return RootComponent ? RootComponent->GetWorldScale() : FVector(1, 1, 1);
}

FMatrix AActor::GetWorldMatrix() const
{
    return RootComponent ? RootComponent->GetWorldMatrix() : FMatrix::Identity();
}

void AActor::AddActorWorldRotation(const FQuat& InDeltaRotation) const
{
    if (RootComponent)
    {
        RootComponent->AddWorldRotation(InDeltaRotation);
    }
}

void AActor::AddActorWorldLocation(const FVector& InDeltaLocation) const
{
    if (RootComponent)
    {
        RootComponent->AddWorldOffset(InDeltaLocation);
    }
}

void AActor::AddActorLocalRotation(const FQuat& InDeltaRotation) const
{
    if (RootComponent)
    {
        RootComponent->AddLocalRotation(InDeltaRotation);
    }
}

void AActor::AddActorLocalLocation(const FVector& InDeltaLocation) const
{
    if (RootComponent)
    {
        RootComponent->AddLocalOffset(InDeltaLocation);
    }
}

const TSet<UActorComponent*>& AActor::GetComponents() const
{
    return OwnedComponents;
}

void AActor::AddComponent(USceneComponent* InComponent)
{
    if (!InComponent)
    {
        return;
    }

    // Set component owner
    InComponent->SetOwner(this);

    OwnedComponents.Add(InComponent);
    if (!RootComponent)
    {
        RootComponent = InComponent;
    }

    // 컴포넌트를 레벨에 등록 (World가 있는 경우)
    if (GetWorld() && !InComponent->bIsRegistered)
    {
        InComponent->RegisterComponent();
    }
}

void AActor::RegisterAllComponents()
{
    for (UActorComponent* Component : OwnedComponents)
    {
        if (Component && !Component->bIsRegistered)
        {
            Component->RegisterComponent();
        }
    }
}

UWorld* AActor::GetWorld() const
{
    // TODO(KHJ): Level 생기면 붙일 것
    // ULevel* Level = GetOuter();
    // if (Level)
    // {
    //     return Level->GetWorld();
    // }

    // return nullptr;

    return World;
}

// ParentComponent 하위에 새로운 컴포넌트를 추가합니다
USceneComponent* AActor::CreateAndAttachComponent(USceneComponent* ParentComponent, UClass* ComponentClass)
{
    // 부모가 지정되지 않았다면 루트 컴포넌트를 부모로 삼습니다.
    if (!ParentComponent)
    {
        ParentComponent = GetRootComponent();
    }

    if (!ComponentClass || !ParentComponent)
    {
        return nullptr;
    }

    // 생성, 등록, 부착 로직을 액터가 직접 책임지고 수행합니다.
    USceneComponent* NewComponent = nullptr;

    if (UObject* NewComponentObject = NewObject(ComponentClass))
    {
        if (NewComponent = Cast<USceneComponent>(NewComponentObject))
        {
            NewComponent->SetOwner(this);
            this->AddComponent(NewComponent); // 액터의 관리 목록에 추가 (내부에서 RegisterComponent 호출)

            NewComponent->SetupAttachment(ParentComponent, EAttachmentRule::KeepRelative);
        }
    }

    return NewComponent;
}

UActorComponent* AActor::AddComponentByClass(UClass* ComponentClass)
{
    if (!ComponentClass)
    {
        return nullptr;
    }
    
    if (ComponentClass->IsChildOf(USceneComponent::StaticClass()))
    {
        return nullptr;
    }

    UActorComponent* NewComp = Cast<UActorComponent>(NewObject(ComponentClass));
    if (NewComp)
    {
        OwnedComponents.Add(NewComp);
        NewComp->SetOwner(this);
        NewComp->OnRegister();
    }
    return NewComp;
}

bool AActor::DeleteComponent(UActorComponent* ComponentToDelete)
{
    // 1. [유효성 검사] nullptr이거나 이 액터가 소유한 컴포넌트가 아니면 실패 처리합니다.
    if (!ComponentToDelete || !OwnedComponents.Contains(ComponentToDelete))
    {
        return false;
    }

    // +-+-+ Only for Scene Component +-+-+
    if (USceneComponent* SceneCompToDelete = Cast<USceneComponent>(ComponentToDelete))
    {
        // 2a. [루트 컴포넌트 보호] 루트 컴포넌트는 액터의 기준점이므로, 직접 삭제하는 것을 막습니다.
        // 루트를 바꾸려면 다른 컴포넌트를 루트로 지정하는 방식을 사용해야 합니다.
        if (SceneCompToDelete == RootComponent)
        {
            UE_LOG("루트 컴포넌트는 직접 삭제할 수 없습니다.");
            return false;
        }

        // 2b. [자식 컴포넌트 처리] 삭제될 컴포넌트의 자식들을 조부모에게 재연결합니다.
        if (USceneComponent* ParentOfDoomedComponent = SceneCompToDelete->GetAttachParent())
        {
            // 자식 목록의 복사본을 만들어 순회합니다. (원본을 수정하면서 순회하면 문제가 발생)
            TArray<USceneComponent*> ChildrenToReAttach = SceneCompToDelete->GetAttachChildren();
            for (USceneComponent* Child : ChildrenToReAttach)
            {
                // 자식을 조부모에게 다시 붙입니다.
                Child->SetupAttachment(ParentOfDoomedComponent);
            }
        }

        // 2c. [부모로부터 분리] 이제 삭제될 컴포넌트를 부모로부터 분리합니다.
        SceneCompToDelete->DetachFromParent();
    }

    // +-+-+ Common Logic +-+-+

    // 3. [소유 목록에서 제거] 액터의 관리 목록에서 포인터를 제거합니다.
    //    이걸 하지 않으면 액터 소멸자에서 이미 삭제된 메모리에 접근하여 충돌합니다.
    OwnedComponents.Remove(ComponentToDelete);

    // 4. [메모리 해제] 모든 연결이 정리되었으므로, 마지막으로 객체를 삭제합니다.
    ObjectFactory::DeleteObject(ComponentToDelete);

    // 5. [레벨 캐시 무효화] 레벨의 ComponentCache를 클리어하여 삭제된 컴포넌트 참조 방지
    if (UWorld* CurrentWorld = GetWorld())
    {
        if (ULevel* CurrentLevel = CurrentWorld->GetLevel())
        {
            CurrentLevel->ClearComponentCache();
        }
    }

    return true;
}

UObject* AActor::Duplicate()
{
    // 원본(this)의 RootComponent 저장
    USceneComponent* OriginalRoot = this->RootComponent;

    // 얕은 복사 수행 (생성자 실행됨)
    AActor* DuplicateActor = NewObject<AActor>(*this);
    DuplicateActor->SetName(GetName().ToString());

    // 생성자가 만든 RootComponent 삭제
    if (DuplicateActor->RootComponent)
    {
        DuplicateActor->OwnedComponents.Remove(DuplicateActor->RootComponent);
        ObjectFactory::DeleteObject(DuplicateActor->RootComponent);
        DuplicateActor->RootComponent = nullptr;
    }
    DuplicateActor->OwnedComponents.clear();

    // 원본의 RootComponent 복제
    if (OriginalRoot)
    {
        DuplicateActor->RootComponent = Cast<USceneComponent>(OriginalRoot->Duplicate());
    }

    // Non-Scene Component만 따로 복제
    for (UActorComponent* OriginalComponent : this->OwnedComponents)
    {
        if (OriginalComponent && !Cast<USceneComponent>(OriginalComponent))
        {
            UActorComponent* DuplicateNonSceneComp = Cast<UActorComponent>(OriginalComponent->Duplicate());
            if (DuplicateNonSceneComp)
            {
                DuplicateNonSceneComp->SetOwner(DuplicateActor);
                DuplicateActor->OwnedComponents.Add(DuplicateNonSceneComp);
            }
        }
    }

    // OwnedComponents 재구성
    DuplicateActor->DuplicateSubObjects();

    // 복제된 컴포넌트는 아직 레벨에 등록되지 않았으므로 bIsRegistered를 false로 설정
    // SpawnActor 호출 시 자동으로 RegisterComponent가 호출됨
    for (UActorComponent* Comp : DuplicateActor->GetComponents())
    {
        if (Comp)
        {
            Comp->bIsRegistered = false;
        }
    }

    return DuplicateActor;
}

/**
 * @brief Actor의 Internal 복사 함수
 * 원본이 들고 있던 Component를 각 Component의 복사함수를 호출하여 받아온 후 새로 담아서 처리함
 */
void AActor::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();

    // Duplicate()에서 이미 RootComponent를 복제했으므로
    // 여기서는 OwnedComponents만 재구성
    if (RootComponent)
    {
        TQueue<USceneComponent*> Queue;
        Queue.Enqueue(RootComponent);
        while (Queue.size() > 0)
        {
            USceneComponent* Component = Queue.front();
            Queue.pop();
            Component->SetOwner(this);
            OwnedComponents.Add(Component);

            for (USceneComponent* Child : Component->GetAttachChildren())
            {
                Queue.Enqueue(Child);
            }
        }
    }

    //TSet<UActorComponent*> DuplicatedComponents = OwnedComponents;
    //OwnedComponents.Empty();
    //
    //USceneComponent* NewRootComponent = nullptr;

    //for (UActorComponent* Component : DuplicatedComponents)
    //{
    //    //USceneComponent* NewComponent = Component->Duplicate<USceneComponent>();
    //    USceneComponent* NewComponent = Cast<USceneComponent>(Component->Duplicate());
    //    
    //    // 복제된 컴포넌트의 Owner를 현재 액터로 설정
    //    if (NewComponent)
    //    {
    //        NewComponent->SetOwner(this);
    //    }
    //    
    //    OwnedComponents.Add(NewComponent);
    //    
    //    if (Component == RootComponent)
    //    {
    //        NewRootComponent = NewComponent;
    //    }
    //}
    //
    //// RootComponent 업데이트
    //RootComponent = NewRootComponent;
}

void AActor::DestroyAllComponents()
{
    if (!RootComponent)
        return;

    // 재귀적으로 Attach 트리를 삭제
    std::function<void(USceneComponent*)> DestroyRecursive = [&](USceneComponent* Comp)
        {
            if (!Comp) return;

            // 자식 목록을 복사해서 순회 (Detach 중 원본 수정 방지)
            const TArray<USceneComponent*> Children = Comp->GetAttachChildren();
            for (USceneComponent* Child : Children)
            {
                DestroyRecursive(Child);
            }

            // 부모로부터 분리
            Comp->DetachFromParent();

            // OwnedComponents에서 제거
            OwnedComponents.Remove(Comp);

            // 객체 소멸 (ObjectFactory 관리 하에 안전하게 삭제)
            ObjectFactory::DeleteObject(Comp);
        };

    // 루트부터 전체 삭제
    DestroyRecursive(RootComponent);
    RootComponent = nullptr;
}

