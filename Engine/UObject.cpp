#include "stdafx.h"
#include "UObject.h"
#include "UClass.h"

IMPLEMENT_ROOT_UCLASS(UObject)

// 유니크 이름 생성
FString UObject::GenerateUniqueName(const FString& base)
{
    // base 자체를 그대로 쓰지 않고, 항상 접미사 0부터 시작
    uint32& next = NameSuffixCounters[base]; // 미존재 시 0으로 초기화됨
    for (;;)
    {
        FString candidate = base + "_" + std::to_string(next++);
        if (LiveNameSet.find(candidate) == LiveNameSet.end())
        {
            return candidate; // 예: "UCubeComp_0", "", -> "_0"
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