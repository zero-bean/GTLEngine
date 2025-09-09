#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<UObject*> GUObjectArray;

UObject::UObject()
	: Name("")
	  , Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

UObject::UObject(const FString& InString)
	: Name(InString)
	  , Outer(nullptr)
{
	UObject();
}
