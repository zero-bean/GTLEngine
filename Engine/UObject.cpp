#include "stdafx.h"
#include "UObject.h"
#include "UClass.h"
#include "UNamePool.h"

IMPLEMENT_ROOT_UCLASS(UObject)

bool UObject::IsNameInUse(const FName& name)
{
    return GAllObjectNames.find(name) != GAllObjectNames.end();
}

void UObject::RegisterName(const FName& name)
{
    if (name.ToString() == "None") return;
    GAllObjectNames.insert(name);
}

void UObject::UnregisterName(const FName& name)
{
    auto it = GAllObjectNames.find(name);
    if (it != GAllObjectNames.end())
    {
        GAllObjectNames.erase(it);
    }
}

// 유니크 이름 생성: 항상 Base_0부터, FName 기반 검사(ComparisonIndex 기준)
FName UObject::GenerateUniqueName(const FString& base)
{
    // 카운터의 키도 FName(ComparisonIndex 기준). 대소문자 다른 Base는 동일 키로 취급.
    FName baseKey(base);

    uint32& next = NameSuffixCounters[baseKey]; // 없으면 0으로 초기화
    for (;;)
    {
        FString candidateStr = base + "_" + std::to_string(next++);
        FName candidate(candidateStr); // UNamePool에 인터닝됨
        if (!IsNameInUse(candidate))
        {
            return candidate;
        }
        // 이미 사용 중이면 다음 번호로
    }
}

void UObject::AssignDefaultNameFromClass(const UClass* cls)
{
    if (!cls) return;

    const FString& currentStr = Name.ToString();
    const bool hasCustom = (!currentStr.empty() && currentStr != "None");

    if (hasCustom)
    {
        // 커스텀 이름이 이미 집합에 없다면 등록만 수행. 있으면 클래스명을 기준으로 재생성.
        if (!IsNameInUse(Name))
        {
            RegisterName(Name);
            return;
        }
        // 충돌 → 아래에서 재네이밍
    }

    const FString& base = cls->GetUClassName();
    FName unique = GenerateUniqueName(base);
    Name = unique;
    RegisterName(Name);
}

void UObject::AssignDefaultName()
{
    AssignDefaultNameFromClass(GetClass());
}