#pragma once
#include "Level/Public/Level.h"
#include "Mesh/Public/Actor.h"

class UGizmoArrowComponent;

class AGizmo : public AActor
{
public:
	AGizmo();
	void SetTargetActor(AActor* actor);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay() override;
private:
	AActor* TargetActor = nullptr;

	UGizmoArrowComponent* GizmoArrowR = nullptr;
	UGizmoArrowComponent* GizmoArrowG = nullptr;
	UGizmoArrowComponent* GizmoArrowB = nullptr;
};
