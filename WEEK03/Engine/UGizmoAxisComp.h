#pragma once
#include "UGizmoComponent.h"

class URenderer;

class UGizmoAxisComp : public UGizmoComponent
{
	DECLARE_UCLASS(UGizmoAxisComp, UGizmoComponent)
public:
	using UGizmoComponent::UGizmoComponent;

	json::JSON Serialize() const override
	{
		return json::JSON();
	}

	UGizmoAxisComp();
};

