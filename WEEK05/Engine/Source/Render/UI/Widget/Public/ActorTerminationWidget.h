#pragma once
#include "Widget.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"

/**
 * @brief 해당 Widget은 ImGui 기능을 제공하지 않지만 Termination 유틸을 제공한다
 */
class UActorTerminationWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void DeleteSelectedActor(TObjectPtr<AActor> InSelectedActor);
	void DeleteSelectedComponent(TObjectPtr<AActor> InSelectedActor, TObjectPtr<UActorComponent> InSelectedComponent);

	UActorTerminationWidget();
	UActorTerminationWidget(UActorDetailWidget* InActorDetailWidget);
	~UActorTerminationWidget() override;

private:
	TObjectPtr<UActorDetailWidget> ActorDetailWidget = nullptr;
};
