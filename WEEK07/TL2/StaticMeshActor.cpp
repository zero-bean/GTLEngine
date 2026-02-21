#include "pch.h"
#include "StaticMeshActor.h"
#include "ObjectFactory.h"

AStaticMeshActor::AStaticMeshActor()
{
    Name = "Static Mesh Actor";
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
    RootComponent = StaticMeshComponent;
}

void AStaticMeshActor::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

    static float times;
    times += DeltaTime;
    //if (World->WorldType == EWorldType::PIE) {
    //    RootComponent->AddLocalRotation({ 0.01f, 0.0f,0.0f });
    //    RootComponent->AddLocalOffset({ sin(times)/100, sin(times)/100,sin(times)/100 });
    //}
    /*if(bIsPicked&& CollisionComponent)
    CollisionComponent->SetFromVertices(StaticMeshComponent->GetStaticMesh()->GetStaticMeshAsset()->Vertices);*/
}

AStaticMeshActor::~AStaticMeshActor()
{
    if (StaticMeshComponent)
    {
        ObjectFactory::DeleteObject(StaticMeshComponent);
    }
    StaticMeshComponent = nullptr;
}

void AStaticMeshActor::SetStaticMeshComponent(UStaticMeshComponent* InStaticMeshComponent)
{
    StaticMeshComponent = InStaticMeshComponent;
}

UObject* AStaticMeshActor::Duplicate()
{
    return Super_t::Duplicate();
}

void AStaticMeshActor::DuplicateSubObjects()
{
    // Duplicate()에서 이미 RootComponent를 복제했으므로
    // 부모 클래스가 OwnedComponents를 재구성
    Super_t::DuplicateSubObjects();

    // 타입별 포인터 재설정
    StaticMeshComponent = Cast<UStaticMeshComponent>(RootComponent);
}

// 특화된 멤버 컴포넌트 CollisionComponent, StaticMeshComponent 는 삭제 시 포인터를 초기화합니다.
bool AStaticMeshActor::DeleteComponent(UActorComponent* ComponentToDelete)
{
    // 2. [부모 클래스의 원래 기능 호출]
    // 기본적인 삭제 로직(소유 목록 제거, 메모리 해제 등)은 부모에게 위임합니다.
    // Super:: 키워드를 사용합니다.
    return AActor::DeleteComponent(ComponentToDelete);
}
