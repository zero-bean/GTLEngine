#include "pch.h"
#include "Component/Public/TextRenderComponent.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(UTextRenderComponent, UPrimitiveComponent);

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UTextRenderComponent::UTextRenderComponent()
{
	POwnerActor = nullptr;
	SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	SetName("TextRenderComponent");
	Type = EPrimitiveType::TextRender;
}

UTextRenderComponent::UTextRenderComponent(AActor* InOwnerActor, float InYOffset)
	: POwnerActor(InOwnerActor)
{
	SetName("TextRenderComponent");
	Type = EPrimitiveType::TextRender;
}

UTextRenderComponent::~UTextRenderComponent()
{
	POwnerActor = nullptr;
}

FString UTextRenderComponent::GetText() const
{
	return Text;
}

void UTextRenderComponent::SetText(const FString& InText)
{
	Text = InText;
}
