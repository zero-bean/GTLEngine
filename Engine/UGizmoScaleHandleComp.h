#pragma once
#include "UGizmoComponent.h"
#include "UGizmoManager.h"


class UGizmoScaleHandleComp : public UGizmoComponent
{
	DECLARE_UCLASS(UGizmoScaleHandleComp, UGizmoComponent)
public:
	using UGizmoComponent::UGizmoComponent;

	EAxis Axis = EAxis::None;

	UGizmoScaleHandleComp();
};