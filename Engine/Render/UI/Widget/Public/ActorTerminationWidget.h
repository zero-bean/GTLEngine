#pragma once
#include "Widget.h"

class UActorTerminationWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void DeleteSelectedActor();

	UActorTerminationWidget();
	~UActorTerminationWidget() override;

private:
	AActor* SelectedActor;
};
