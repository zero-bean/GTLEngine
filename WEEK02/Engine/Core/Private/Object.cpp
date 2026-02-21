#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<UObject*> GUObjectArray;

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = "Object_" + to_string(UUID);

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
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있다면 해당 오브젝트에서 메모리 관리 제거
	if (Outer)
	{
		Outer->RemoveMemoryUsage(AllocatedBytes, AllocatedCounts);
	}

	// 새로운 Outer 설정 후 새로운 Outer에서 메모리 관리
	Outer = InObject;
	if (Outer)
	{
		Outer->AddMemoryUsage(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	AllocatedBytes += InBytes;
	AllocatedCounts += InCount;

	if (Outer)
	{
		Outer->AddMemoryUsage(InBytes);
	}
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	if (AllocatedBytes >= InBytes)
	{
		AllocatedBytes -= InBytes;
	}
	if (AllocatedCounts >= InCount)
	{
		AllocatedCounts -= InCount;
	}

	if (Outer)
	{
		Outer->RemoveMemoryUsage(InBytes);
	}
}

/**
 * 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * Recursive하게 처리함
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(const UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}
