#pragma once
#include "Actor.h"

class APlayerStart : public AActor
{
	DECLARE_CLASS(APlayerStart, AActor)

public:
	void InitializeComponents() override;
};
