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

// 전역 이름 집합 해제 + 접미사 카운터 조정 (남은 이름들 중 가장 큰 접미사 뒤에 붙이도록 보정)
void UObject::UnregisterName(const FName& Name)
{
    // 1) 라이브 이름 집합에서 제거
    auto LiveIt = GAllObjectNames.find(Name);
    if (LiveIt != GAllObjectNames.end())
    {
        GAllObjectNames.erase(LiveIt);
    }

    // 2) "Base" 또는 "Base_<숫자>" 형태에서 Base 부분을 파싱
    const FString NameStr = Name.ToString();
    if (NameStr.empty())
    {
        return; // 비어 있으면 더 할 일 없음
    }

    // Base 문자열 추출
    FString BasePlainStr;
    {
        const size_t UnderscorePos = NameStr.find_last_of('_');
        if (UnderscorePos != FString::npos && UnderscorePos + 1 < NameStr.size())
        {
            const FString SuffixDigitsStr = NameStr.substr(UnderscorePos + 1);
            bool bNumericSuffix = !SuffixDigitsStr.empty();
            for (char ch : SuffixDigitsStr)
            {
                if (ch < '0' || ch > '9') { bNumericSuffix = false; break; }
            }
            // 숫자 접미사면 '_' 앞까지가 Base, 아니면 전체가 Base
            BasePlainStr = bNumericSuffix ? NameStr.substr(0, UnderscorePos) : NameStr;
        }
        else
        {
            BasePlainStr = NameStr; // '_'가 없으면 전체가 Base
        }
    }

    const FName BaseFName(BasePlainStr);

    // 3) 남아 있는 이름들(GAllObjectNames)을 스캔:
    //    - Base와 정확히 동일한 평문 이름이 남아있는지
    //    - "Base_<숫자>" 중 최대 접미사 값이 무엇인지
    bool   bPlainBaseExists = false;
    uint32 MaxSuffixFound = 0;

    // 정확한 평문 비교는 FName 비교로 처리(문자열 변환 최소화)
    const FString BasePrefixStr = BasePlainStr + "_";

    for (const FName& ExistingName : GAllObjectNames)
    {
        // 평문(Base)과 동일하면 FName 비교로 즉시 체크
        if (ExistingName == BaseFName)
        {
            bPlainBaseExists = true;
            continue;
        }

        // 접미사 패턴 확인이 필요한 경우에만 문자열 변환
        const FString ExistingStr = ExistingName.ToString();

        // Base_ 로 시작하는지 확인
        if (ExistingStr.size() <= BasePrefixStr.size())
        {
            continue;
        }
        // starts_with(BasePrefixStr)
        if (ExistingStr.compare(0, BasePrefixStr.size(), BasePrefixStr) != 0)
        {
            continue;
        }

        // 접두사 뒤에 오는 부분이 전부 숫자인지 검사
        const FString SuffixStr = ExistingStr.substr(BasePrefixStr.size());
        bool bDigits = !SuffixStr.empty();
        for (char ch : SuffixStr)
        {
            if (ch < '0' || ch > '9') { bDigits = false; break; }
        }
        if (!bDigits) continue;

        // 최대 접미사 갱신
        uint32 Parsed = 0;
        try { Parsed = static_cast<uint32>(std::stoul(SuffixStr)); }
        catch (...) { Parsed = 0; }
        if (Parsed > MaxSuffixFound)
        {
            MaxSuffixFound = Parsed;
        }
    }

    // 4) Base 계열이 하나도 남아있지 않다면 카운터 제거 → 다음 생성은 평문(Base)부터 시작
    if (!bPlainBaseExists && MaxSuffixFound == 0)
    {
        NameSuffixCounters.erase(BaseFName);
        return;
    }

    // 5) 남아있다면 다음 시도 값 설정:
    //    - 평문만 남아있다면 1부터
    //    - 접미사가 남아있다면 (최대 접미사 + 1)부터
    NameSuffixCounters[BaseFName] = (MaxSuffixFound > 0 ? MaxSuffixFound + 1 : 1);
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