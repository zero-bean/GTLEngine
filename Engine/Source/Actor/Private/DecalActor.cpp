#include "pch.h"
#include "Actor/Public/DecalActor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Texture/Public/Material.h"

IMPLEMENT_CLASS(ADecalActor, AActor)

ADecalActor::ADecalActor()
{
	// 1. 액터의 위치, 회전, 크기를 담당할 루트 컴포넌트 생성
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(FName("RootComponent"));
	SetRootComponent(Root);

	// 2. 데칼의 핵심 기능을 담당할 DecalComponent 생성 및 부착
	if (DecalComponent = CreateDefaultSubobject<UDecalComponent>(FName("DecalComponent")))
	{
		DecalComponent->SetParentAttachment(GetRootComponent());
	}
}

ADecalActor::~ADecalActor()
{

}
