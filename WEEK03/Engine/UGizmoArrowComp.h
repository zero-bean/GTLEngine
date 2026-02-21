#pragma once
#include "UGizmoComponent.h"
#include "UGizmoManager.h"

class UGizmoArrowComp : public UGizmoComponent
{
	DECLARE_UCLASS(UGizmoArrowComp, UGizmoComponent)
public:
	using UGizmoComponent::UGizmoComponent;

	EAxis Axis = EAxis::None;

	UGizmoArrowComp();
};