#pragma once
#include "Level/Public/Level.h"
#include "Mesh/Public/Actor.h"

class UGizmoArrowComponent;

class AGizmo : public AActor
{
public:
	AGizmo();
	~AGizmo() override;
	void SetTargetActor(AActor* actor);

	void BeginPlay() override;
	void Tick() override;
	void EndPlay() override;

private:
	AActor* TargetActor = nullptr;

	UGizmoArrowComponent* GizmoArrowR = nullptr;
	UGizmoArrowComponent* GizmoArrowG = nullptr;
	UGizmoArrowComponent* GizmoArrowB = nullptr;
};
