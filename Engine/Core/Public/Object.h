#pragma once

#include "pch.h"
class UObject
{
public:
	// Special Member Function
    UObject();

	UObject* Outer = nullptr;
	FString Name;
    virtual ~UObject();

private:
	UINT ID;
};
