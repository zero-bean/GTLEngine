#include "pch.h"
#include "Core/Public/Object.h"

static UINT NextID = 0;

UObject::UObject()
	: Name("")
	  , Outer(nullptr)
{
	++NextID;
	ID = NextID;
}

UObject::UObject(const FString& InString)
	: Name(InString)
	  , Outer(nullptr)
{
	++NextID;
	ID = NextID;
}
