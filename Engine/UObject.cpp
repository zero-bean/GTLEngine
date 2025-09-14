#include "stdafx.h"
#include "UObject.h"
#include "UClass.h"
#include "UNamePool.h"

IMPLEMENT_ROOT_UCLASS(UObject)

// 전역 이름 집합 조회
bool UObject::IsNameInUse(const FName& Name)
{
    return GAllObjectNames.find(Name) != GAllObjectNames.end();
}

// 전역 이름 집합 등록(유효 이름만)
void UObject::RegisterName(const FName& Name)
{
    if (Name.ToString() == "None") return;
    GAllObjectNames.insert(Name);
}

// 전역 이름 집합 해제
void UObject::UnregisterName(const FName& Name)
{
    auto It = GAllObjectNames.find(Name);
    if (It != GAllObjectNames.end())
    {
        GAllObjectNames.erase(It);
    }
}

// 유니크 이름 생성: 첫 객체는 Base, 이후 Base_1부터 증가
FName UObject::GenerateUniqueName(const FName& Base)
{
    // 1) 접미사 없는 기본 이름 먼저 시도
    if (!IsNameInUse(Base))
    {
        uint32& NextInit = NameSuffixCounters[Base];
        if (NextInit < 1)
        {
            NextInit = 1;
        }
        return Base;
    }

    // 2) 이미 사용 중이면 1부터 붙여가며 탐색
    uint32& Next = NameSuffixCounters[Base];
    if (Next < 1)
    {
        Next = 1;
    }

    const FString BaseStr = Base.ToString();
    for (;;)
    {
        FString CandidateStr = BaseStr + "_" + std::to_string(Next);
        FName Candidate(CandidateStr); // UNamePool에 인터닝
        if (!IsNameInUse(Candidate))
        {
            ++Next; // 다음 호출을 위해 증가
            return Candidate;
        }

        ++Next; // 충돌이면 계속 증가
    }
}

// 완전 생성 이후 클래스명(FName) 기반으로 고유 이름 부여/등록
void UObject::AssignDefaultNameFromClass(const UClass* Cls)
{
    if (!Cls) return;

    const FString& CurrentStr = Name.ToString();
    const bool bHasCustom = (!CurrentStr.empty() && CurrentStr != "None");

    if (bHasCustom)
    {
        if (!IsNameInUse(Name))
        {
            RegisterName(Name);
            return;
        }
        // 충돌 시 아래에서 재생성
    }

    const FName& Base = Cls->GetUClassName();
    FName Unique = GenerateUniqueName(Base);
    Name = Unique;
    RegisterName(Name);
}

// 편의 함수(완전 생성 이후 가상 호출 사용)
void UObject::AssignDefaultName()
{
    AssignDefaultNameFromClass(GetClass());
}

// 문자열 베이스를 FName으로 변환하여 이름 생성/등록
void UObject::AssignNameFromString(const FString& Base)
{
    const FString& CurrentStr = Name.ToString();
    const bool bHasCustom = (!CurrentStr.empty() && CurrentStr != "None");
    if (bHasCustom)
    {
        if (!IsNameInUse(Name))
        {
            RegisterName(Name);
            return;
        }
    }

    FName Unique = GenerateUniqueName(FName(Base));
    Name = Unique;
    RegisterName(Name);
}

// 디버그/에디터: 현재 라이브 이름을 문자열로 스냅샷
void UObject::CollectLiveObjectNames(TArray<FString>& OutNames)
{
    OutNames.clear();
    for (const FName& N : GAllObjectNames)
    {
        OutNames.push_back(N.ToString());
    }
}