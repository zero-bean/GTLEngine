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

    // 접미사가 있는 경우(Base_N) NameSuffixCounters를 낮춰서 재사용 가능하게 함
    const FString S = Name.ToString();
    if (!S.empty())
    {
        const size_t Us = S.find_last_of('_');
        if (Us != FString::npos && Us + 1 < S.size())
        {
            const FString SuffixStr = S.substr(Us + 1);
            bool bAllDigits = !SuffixStr.empty();
            for (char c : SuffixStr)
            {
                if (c < '0' || c > '9') 
                { 
                    bAllDigits = false; 
                    break; 
                }
            }

            if (bAllDigits)
            {
                uint32 FreedSuffix = 0;
                try 
                { 
                    FreedSuffix = static_cast<uint32>(std::stoul(SuffixStr)); 
                }
                catch (...) 
                { 
                    FreedSuffix = 0; 
                }

                if (FreedSuffix > 0)
                {
                    const FName Base(S.substr(0, Us));
                    auto CIt = NameSuffixCounters.find(Base);
                    if (CIt == NameSuffixCounters.end())
                    {
                        // 카운터가 없다면 이번에 해제된 접미사부터 시도하도록 설정
                        NameSuffixCounters[Base] = FreedSuffix;
                    }
                    else
                    {
                        // 다음 시도 값이 더 크면 낮춰서 방금 해제된 번호를 재사용
                        if (CIt->second > FreedSuffix)
                        {
                            CIt->second = FreedSuffix;
                        }
                    }
                }
            }
			// 평문 Base 해제 시 Base를 다시 사용 가능하게 카운터 제거
            else 
            {
                const FName Base(S);
                NameSuffixCounters.erase(Base);
            }
        }
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