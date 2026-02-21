#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"
#include "Core/Public/Name.h"
#include "Core/Public/ObjectIterator.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

uint32 UEngineStatics::NextUUID = 0;

TArray<TObjectPtr<UObject>>& GetUObjectArray()
{
	static TArray<TObjectPtr<UObject>> GUObjectArray;
	return GUObjectArray;
}

IMPLEMENT_CLASS_BASE(UObject)

UObject::~UObject()
{
	/** @todo: 이후에 리뷰 필요 */

	// std::vector에 맞는 올바른 인덱스 유효성 검사
	if (InternalIndex < GetUObjectArray().size())
	{
		GetUObjectArray()[InternalIndex] = nullptr;
	}

	// 객체 삭제 시 모든 캐시 무효화 (안전성 우선)
	FObjectCacheManager::InvalidateCache();
}

UObject::UObject()
	: Name(FName::GetNone()), Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = FName("Object_" + to_string(UUID));

	InternalIndex = static_cast<uint32>(GetUObjectArray().size());
	GetUObjectArray().emplace_back(this);
}

UObject::UObject(const FName& InName)
	: Name(InName)
	, Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	InternalIndex = static_cast<uint32>(GetUObjectArray().size());
	GetUObjectArray().emplace_back(this);
}

void UObject::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("Name"))
		{
			SetName(FName(InOutHandle["Name"].ToString()));
		}
	}
	else
	{
		InOutHandle["Name"] = GetName().ToString();
	}
}

UObject* UObject::Duplicate(FObjectDuplicationParameters Parameters)
{
	/** @note 이미 오브젝트가 존재할 경우 복제하지 않음 */
	if (auto It = Parameters.DuplicationSeed.find(Parameters.SourceObject); It != Parameters.DuplicationSeed.end())
	{
		return It->second;
	}

	UObject* DupObject = Parameters.DestClass->CreateDefaultObject();

	DupObject->SetOuter(Parameters.DestOuter);
	DupObject->GenerateDuplicatedObjectName(GetName(), DupObject->GetUUID());

	/** @note 새로운 오브젝트가 생성되었을 경우 맵을 업데이트 해준다. */
	Parameters.DuplicationSeed.emplace(Parameters.SourceObject, DupObject);
	Parameters.CreatedObjects.emplace(Parameters.SourceObject, DupObject);

	return DupObject;
}

void UObject::DuplicateSubObjects(FObjectDuplicationParameters Parameters)
{
	return;
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있었다면, 나의 전체 메모리 사용량을 빼달라고 전파
	// 새로운 Outer가 있다면, 나의 전체 메모리 사용량을 더해달라고 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(-static_cast<int64>(AllocatedBytes), -static_cast<int32>(AllocatedCounts));
	}

	Outer = InObject;

	if (Outer)
	{
		Outer->PropagateMemoryChange(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	uint64 BytesToAdd = InBytes;

	if (!BytesToAdd)
	{
		BytesToAdd = GetClass()->GetClassSize();
	}

	// 메모리 변경 전파
	PropagateMemoryChange(BytesToAdd, InCount);
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	PropagateMemoryChange(-static_cast<int64>(InBytes), -static_cast<int32>(InCount));
}

std::string UObject::GenerateDuplicatedObjectName(const FName& OriginalName, const uint32& NewUUID)
{
	string BaseName = OriginalName.ToString();
	string BaseUUID = to_string(NewUUID);
	size_t UnderBarIndex = BaseName.find_last_of('_');

	// 마지막 '_' 문자를 찾았고, 그 뒤에 글자가 있는지 확인
	if (UnderBarIndex != string::npos && UnderBarIndex < BaseName.length() - 1)
	{
		string Suffix = BaseName.substr(UnderBarIndex + 1);

		// 접미사가 숫자로만 이루어져 있는지 확인
		bool bSuffixNumeric = !Suffix.empty(); 
		for (char C : Suffix)
		{
			if (!std::isdigit(C))
			{
				bSuffixNumeric = false;
				break;
			}
		}

		// 접미사가 숫자라면, '_' 앞부분까지만 이름으로 사용
		if (bSuffixNumeric)
		{
			BaseName = BaseName.substr(0, UnderBarIndex);
		}
	}

	// 정리된 이름에 새 UUID를 붙여 반환
	return BaseName + "_" + BaseUUID;
}

void UObject::PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta)
{
	// 자신의 값에 변화량을 더함
	AllocatedBytes += InBytesDelta;
	AllocatedCounts += InCountDelta;

	// Outer가 있다면, 동일한 변화량을 그대로 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(InBytesDelta, InCountDelta);
	}
}

/**
 * @brief 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * 내부적으로 재귀를 활용해서 부모를 계속 탐색한 뒤 결과를 반환한다
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(TObjectPtr<UClass> InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}
