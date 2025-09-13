#include "stdafx.h"
#include "UObject.h"
#include "UClass.h"

IMPLEMENT_ROOT_UCLASS(UObject)

// 유니크 이름 생성
FString UObject::GenerateUniqueName(const FString& base)
{
    // base 미사용이면 그대로 사용
    if (LiveNameSet.find(base) == LiveNameSet.end())
    {
        return base;
    }

    // 이미 있으면 Base_0, Base_1, ... 식으로
    uint32& next = NameSuffixCounters[base]; // 처음 접근 시 0으로 디폴트 생성
    for (;;)
    {
        FString candidate = base + "_" + std::to_string(next++);
        if (LiveNameSet.find(candidate) == LiveNameSet.end())
        {
            return candidate;
        }
    }
}

void UObject::RegisterLiveName(const FString& name)
{
    LiveNameSet[name] = 1;
}

void UObject::UnregisterLiveName(const FString& name)
{
    auto it = LiveNameSet.find(name);
    if (it != LiveNameSet.end())
    {
        LiveNameSet.erase(it);
    }
}

void UObject::AssignDefaultNameFromClass(const UClass* cls)
{
    if (!cls) 
    {
        return;
    }

    // 기본은 클래스명. 필요하면 GetDisplayName()로 교체 가능.
    const FString& base = cls->GetUClassName();

    // 이미 커스텀 이름이 있으면 덮어쓰지 않음
    const FString& current = Name.ToString();
    if (!current.empty() && current != "None")
    {
        return;
    }

    FString unique = GenerateUniqueName(base);
    Name = FName(unique);       // UNamePool 등록
    RegisterLiveName(unique);   // 라이브 네임 트래킹
}

void UObject::AssignDefaultName()
{
    // 객체가 완전히 생성된 이후에만 사용
    AssignDefaultNameFromClass(GetClass());
}