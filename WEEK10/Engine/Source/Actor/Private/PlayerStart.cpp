#include "pch.h"
#include "Actor/Public/PlayerStart.h"
#include "Component/Public/EditorIconComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(APlayerStart, AActor)

void APlayerStart::InitializeComponents()
{
	Super::InitializeComponents();

	UEditorIconComponent* Icon = AddComponent<UEditorIconComponent>();
	if (!Icon) { return; }
	Icon->AttachToComponent(GetRootComponent());
	Icon->SetIsVisualizationComponent(true);
	Icon->SetSprite(UAssetManager::GetInstance().LoadTexture("Asset/Icon/PlayerStart.png"));
	Icon->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));
	Icon->SetScreenSizeScaled(true);
}
