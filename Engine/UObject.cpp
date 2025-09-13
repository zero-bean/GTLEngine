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

// 유니크 이름 생성: 첫 객체는 Base, 이후 Base_1부터 시작
FName UObject::GenerateUniqueName(const FString& base)
{
    FName baseKey(base);

    // 1) 접미사 없는 기본 이름 먼저 시도
    FName plain(base);
    if (!IsNameInUse(plain))
    {
        // 다음부터는 1부터 붙이도록 카운터 최소값 보장
        uint32& nextInit = NameSuffixCounters[baseKey];
        if (nextInit < 1)
        {
            nextInit = 1;
        }

        return plain;
    }

    // 2) 이미 사용 중이면 1부터 붙여가며 탐색
    uint32& next = NameSuffixCounters[baseKey];
    if (next < 1) 
    {
        next = 1;
    }

    for (;;)
    {
        FString candidateStr = base + "_" + std::to_string(next);
        FName candidate(candidateStr); // UNamePool에 인터닝됨
        if (!IsNameInUse(candidate))
        {
            ++next; // 다음 호출을 위해 증가
            return candidate;
        }
        
        ++next; // 충돌이면 계속 증가
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