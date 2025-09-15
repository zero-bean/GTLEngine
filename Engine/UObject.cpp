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
    if (Name == "None") 
    {
        return;
    }

    GAllObjectNames.insert(Name);
}

// 전역 이름 집합 해제 + 접미사 카운터 조정
void UObject::UnregisterName(const FName& Name)
{
    auto It = GAllObjectNames.find(Name);
    if (It != GAllObjectNames.end())
    {
        GAllObjectNames.erase(It);
    }

    // 이름에서 Base와 숫자 접미사를 파싱
    const FString S = Name.ToString();
    if (S.empty()) 
    {
        return;
    }

    auto ParseBaseAndSuffix = [](const FString& In, FString& OutBase, uint32& OutNum, bool& bHasNumSuffix)
        {
            OutBase.clear(); OutNum = 0; bHasNumSuffix = false;
            const size_t Us = In.find_last_of('_');
            if (Us != FString::npos && Us + 1 < In.size())
            {
                const FString SuffixStr = In.substr(Us + 1);
                bool bAllDigits = !SuffixStr.empty();
                for (char Ch : SuffixStr)
                {
                    if (Ch < '0' || Ch > '9') { bAllDigits = false; break; }
                }
                if (bAllDigits)
                {
                    try 
                    { 
                        OutNum = static_cast<uint32>(std::stoul(SuffixStr)); 
                    }
                    catch (...) 
                    { 
                        OutNum = 0; 
                    }
                    
                    OutBase = In.substr(0, Us);
                    bHasNumSuffix = (OutNum > 0);
                    
                    return;
                }
            }
            // 접미사 숫자가 없으면 전체가 Base
            OutBase = In;
            bHasNumSuffix = false;
        };

    FString BaseStr;
    uint32 FreedSuffix = 0;
    bool bHasNumSuffix = false;
    ParseBaseAndSuffix(S, BaseStr, FreedSuffix, bHasNumSuffix);

    // 숫자 접미사가 있는 경우: 방금 해제된 번호를 우선 재사용하도록 카운터를 낮춤
    if (bHasNumSuffix && FreedSuffix > 0)
    {
        const FName Base(BaseStr);
        auto CIt = NameSuffixCounters.find(Base);
        if (CIt == NameSuffixCounters.end())
        {
            NameSuffixCounters[Base] = FreedSuffix;
        }
        else if (CIt->second > FreedSuffix)
        {
            CIt->second = FreedSuffix;
        }
    }

    // Base 계열 이름이 더 남아있는지 검사: Base 또는 Base_숫자
    bool bAnyLeft = false;
    const FString Prefix = BaseStr + "_";
    for (const FName& N : GAllObjectNames)
    {
        const FString T = N.ToString();
        if (T == BaseStr)
        {
            bAnyLeft = true; break;
        }
        if (T.size() > Prefix.size() && T.rfind(Prefix, 0) == 0) // starts_with
        {
            // 엄밀히 숫자만인지 확인
            bool bAllDigits = true;
            for (char Ch : T.substr(Prefix.size()))
            {
                if (Ch < '0' || Ch > '9') 
                { 
                    bAllDigits = false; 
                    break; 
                }
            }
            if (bAllDigits) { bAnyLeft = true; break; }
        }
    }

    // 더 이상 Base 계열이 없으면 카운터 리셋 → 다음 생성에서 평문(Base)부터 시작
    if (!bAnyLeft)
    {
        NameSuffixCounters.erase(FName(BaseStr));
    }
}

// 유니크 이름 생성: 첫 객체는 Base, 이후 Base_1부터 증가
FName UObject::GenerateUniqueName(const FName& Base)
{
    // Base를 한 번이라도 본 적이 있는지 확인
    auto It = NameSuffixCounters.find(Base);
    if (It == NameSuffixCounters.end())
    {
        // 처음 보는 Base면 평문을 한 번만 허용
        if (!IsNameInUse(Base))
        {
            NameSuffixCounters[Base] = 1; // 다음은 _1부터
            return Base;
        }
        // 이미 사용 중이면 접미사 로직으로 진입
        NameSuffixCounters[Base] = 1;
    }

    // 한 번이라도 본 Base면 평문을 건너뛰고 접미사부터 시작
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
            ++Next; // 다음 호출 준비
            return Candidate;
        }
        ++Next;
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