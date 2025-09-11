#pragma once
#include "UGizmoComponent.h"
#include "UGizmoManager.h"

class UGizmoRotationHandleComp : public UGizmoComponent
{
	DECLARE_UCLASS(UGizmoRotationHandleComp, UGizmoComponent)
public:
	using UGizmoComponent::UGizmoComponent;

	EAxis Axis = EAxis::None;

	UGizmoRotationHandleComp();
};