#include "stdafx.h"
#include "UNamePool.h"
#include "UEngineStatics.h"

// =============================
// FNameEntry
// =============================

FNameEntry::FNameEntry(const FString& InString, int32 InComparisonIndex)
    : String(InString), ComparisonIndex(InComparisonIndex) 
{}

// =============================
// UNamePool (Singleton)
// =============================

UNamePool& UNamePool::GetInstance()
{
    static UNamePool Instance; // thread-safe (C++11 이상)
    return Instance;
}

void UNamePool::FindOrAddEntry(const FString& InString, int32& OutDisplayIndex, int32& OutComparisonIndex)
{
    FString OriginalStr(InString);

    // 1. DisplayIndex 찾기 (대소문자 구분)
    auto DisplayIt = DisplayMap.find(OriginalStr);
    if (DisplayIt != DisplayMap.end())
    {
        // 이미 완전히 동일한 문자열이 존재함
        OutDisplayIndex = DisplayIt->second;
        OutComparisonIndex = Entries[OutDisplayIndex].ComparisonIndex;
        return;
    }

    // 2. 새로운 DisplayIndex이므로, ComparisonIndex 찾기 (대소문자 미구분)
    FString LowerStr = OriginalStr;
    std::transform(LowerStr.begin(), LowerStr.end(), LowerStr.begin(),
        [](unsigned char c) { return std::tolower(c); });

    auto ComparisonIt = ComparisonMap.find(LowerStr);
    if (ComparisonIt != ComparisonMap.end())
    {
        // 이미 같은 철자의 다른 이름이 존재함 (예: "Player"가 있는데 "player"가 들어온 경우)
        OutComparisonIndex = ComparisonIt->second;
    }
    else
    {
        // 완전히 새로운 이름
        // 새로운 ComparisonIndex는 현재 Display 테이블의 크기(다음 인덱스)로 지정
        OutComparisonIndex = static_cast<int32>(Entries.size());
        ComparisonMap[LowerStr] = OutComparisonIndex;
    }

    // 3. 새로운 FNameEntry를 생성하고 테이블에 추가
    int32 NewDisplayIndex = static_cast<int32>(Entries.size());
    Entries.emplace_back(OriginalStr, OutComparisonIndex);

    // 4. DisplayMap에 새로운 인덱스 등록
    DisplayMap[OriginalStr] = NewDisplayIndex;

    OutDisplayIndex = NewDisplayIndex;
}

const FString& UNamePool::GetString(int32 DisplayIndex) const
{
    if (DisplayIndex >= 0 && DisplayIndex < Entries.size())
    {
        return Entries[DisplayIndex].String;
    }

    static const FString NoneString("None");
    
    return NoneString;
}

void UNamePool::LogDebugState() const
{
	UE_LOG("UNamePool State:");
    for (uint32 i = 0; i < Entries.size(); ++i)
    {
		const FNameEntry& entry = Entries[i];
		UE_LOG("  [%d] String: '%s', ComparisonIndex: %d", i, entry.String.c_str(), entry.ComparisonIndex);
    }
}

// =============================
// FName
// =============================

FName::FName(const char* InString)
    : DisplayIndex(-1), ComparisonIndex(-1)
{
    UNamePool::GetInstance().FindOrAddEntry(InString, DisplayIndex, ComparisonIndex);
}

FName::FName(const FString& InName)
	: FName(InName.c_str())
{}

int32 FName::Compare(const FName& Other) const
{
    // ComparisonIndex 가 동일하면 동등 (0), 아니면 인덱스 차이
    if (ComparisonIndex == Other.ComparisonIndex) 
    {
        return 0;
    }

	return (ComparisonIndex < Other.ComparisonIndex) ? -1 : 1;
}

bool FName::operator==(const FName& Other) const
{
    return ComparisonIndex == Other.ComparisonIndex;
}

bool FName::operator!=(const FName& Other) const
{
    return !(*this == Other);
}

const FString& FName::ToString() const
{
    return UNamePool::GetInstance().GetString(DisplayIndex);
}

FName FName::GetNone() const
{
	return FName("None");
}

