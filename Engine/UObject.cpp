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

// 전역 이름 집합 해제 + 접미사 카운터 조정 (가장 큰 Suffix 뒤에 붙이기)
void UObject::UnregisterName(const FName& Name)
{
    auto It = GAllObjectNames.find(Name);
    if (It != GAllObjectNames.end())
    {
        GAllObjectNames.erase(It);
    }

    const FString S = Name.ToString();
    if (S.empty())
    {
        return;
    }

    // Base 추출 (Base 또는 Base_N)
    FString BaseStr;
    {
        const size_t Us = S.find_last_of('_');
        if (Us != FString::npos && Us + 1 < S.size())
        {
            const FString SuffixStr = S.substr(Us + 1);
            bool bAllDigits = !SuffixStr.empty();
            for (char Ch : SuffixStr)
            {
                if (Ch < '0' || Ch > '9') 
                { 
                    bAllDigits = false; 
                    break; 
                }
            }
            BaseStr = bAllDigits ? S.substr(0, Us) : S;
        }
        else
        {
            BaseStr = S;
        }
    }

    // 잔존 이름들 중 Base 계열의 최대 접미사와 평문 존재 여부를 스캔
    bool bPlainLeft = false;
    uint32 MaxSuffix = 0;
    const FString Prefix = BaseStr + "_";
    for (const FName& N : GAllObjectNames)
    {
        const FString T = N.ToString();
        if (T == BaseStr)
        {
            bPlainLeft = true;
            continue;
        }
        if (T.size() > Prefix.size() && T.rfind(Prefix, 0) == 0) // starts_with
        {
            const FString R = T.substr(Prefix.size());
            bool bDigits = !R.empty();
            for (char Ch : R)
            {
                if (Ch < '0' || Ch > '9') 
                { 
                    bDigits = false; 
                    break; 
                }
            }
            if (bDigits)
            {
                uint32 V = 0;
                try 
                { 
                    V = static_cast<uint32>(std::stoul(R)); 
                }
                catch (...) 
                { 
                    V = 0; 
                }
                
                if (V > MaxSuffix) 
                {
                    MaxSuffix = V;
                }
            }
        }
    }

    // 아무 것도 안 남았으면 카운터 제거 → 다음 생성은 평문부터
    if (!bPlainLeft && MaxSuffix == 0)
    {
        NameSuffixCounters.erase(FName(BaseStr));
        return;
    }

    // 남아있다면 다음 시도 값은 (현재 최대 접미사 + 1)
    // (가장 큰 번호를 지운 경우엔 자연스럽게 그 번호가 다시 시도됨)
    NameSuffixCounters[FName(BaseStr)] = (MaxSuffix > 0 ? MaxSuffix + 1 : 1);
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