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
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject) return;

	if (Outer)
	{
		Outer->RemoveMemoryUsage(AllocatedBytes, AllocatedCounts);
	}
	Outer = InObject;
	if (Outer)
	{
		Outer->AddMemoryUsage(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 Bytes, uint32 Count)
{
	AllocatedBytes += Bytes;
	AllocatedCounts += Count;
	if (Outer)
	{
		Outer->AddMemoryUsage(Bytes);
	}
}

void UObject::RemoveMemoryUsage(uint64 Bytes, uint32 Count)
{
	if (AllocatedBytes >= Bytes) AllocatedBytes -= Bytes;
	if (AllocatedCounts >= Count) AllocatedCounts -= Count;
	if (Outer)
	{
		Outer->RemoveMemoryUsage(Bytes);
	}
}
