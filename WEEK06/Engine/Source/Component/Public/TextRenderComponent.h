#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Camera.h"
#include "Global/Matrix.h"

class AActor;

UCLASS()

class UTextRenderComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTextRenderComponent, UPrimitiveComponent)

public:
	UTextRenderComponent();
	UTextRenderComponent(AActor* InOwnerActor, float InYOffset);
	~UTextRenderComponent();

	FString GetText() const;
	void SetText(const FString& InText);

	bool bIsUUIDText = false;

private:
	FString Text;
	AActor* POwnerActor;
};
