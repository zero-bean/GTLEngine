#include "pch.h"
#include "Core/Public/Name.h"
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower

/**@brief FName::None으로 초기화합니다.*/
FName::FName() : ComparisonIndex(0), DisplayIndex(0), Number(-1)
{
}

FName::FName(const FString& Str)
{
    TPair<int32, int32> Indices = FNameTable::GetInstance().FindOrAddName(Str);
    ComparisonIndex = Indices.first;
    DisplayIndex = Indices.second;
    Number = -1;
}

FName::FName(const char* Str) : FName(FString(Str)) { }

/**@brief NameTable에서 UniqueName을 만들 때 사용하는 생성자*/
FName::FName(int32 InComparisonIndex, int32 InDisplayIndex, int32 InNumber)
    : ComparisonIndex(InComparisonIndex), DisplayIndex(InDisplayIndex), Number(InNumber)
{
}

bool FName::operator==(const FName& Other) const
{
    return ComparisonIndex == Other.ComparisonIndex && Number == Other.Number;
}

int32 FName::Compare(const FName& Other) const
{
    if (*this == Other) { return 0; }

    // ComparisonIndex로 먼저 비교
    if (ComparisonIndex < Other.ComparisonIndex) { return -1; }
    if (ComparisonIndex > Other.ComparisonIndex) { return 1; }

    // ComparisonIndex가 같으면 Number로 비교
    if (Number < Other.Number) { return -1; }
    if (Number > Other.Number) { return 1; }

    return 0;
}

FString FName::ToString() const
{
    FString BaseName = FNameTable::GetInstance().GetDisplayString(DisplayIndex);
    if (Number >= 0)
    {
        return BaseName + "_" + to_string(Number);
    }
    return BaseName;
}

FString FName::ToBaseNameString() const
{
    return FNameTable::GetInstance().GetDisplayString(DisplayIndex);
}

const FName FName::None(0, 0, -1); 

// FNameTable

FNameTable::FNameTable()
{
    ComparisonMap["None"] = 0;
    DisplayMap["None"] = 0;
}

FNameTable::~FNameTable() = default;

FNameTable& FNameTable::GetInstance()
{
    static FNameTable Instance;
    return Instance;
}

/**
* @brief FString 객체를 받아 풀에 없으면 반환
* @param Str FName으로 등록되었는지 확인할 FString
* @return ComparisonIndex, DisplayIndex
*/
TPair<int32, int32> FNameTable::FindOrAddName(const FString& Str)
{
    FString LowerStr = ToLower(Str);

    int32 ComparisonIndex;
    auto ItComparison = ComparisonMap.find(LowerStr);
    if (ItComparison != ComparisonMap.end())
    {
        ComparisonIndex = ItComparison->second;
    }
    else
    {
        ComparisonIndex = ComparisonStringPool.size();
        ComparisonStringPool.push_back(LowerStr);
        ComparisonMap[LowerStr] = ComparisonIndex;
    }

    int32 DisplayIndex;
    auto ItDisplay = DisplayMap.find(Str);
    if (ItDisplay != DisplayMap.end())
    {
        DisplayIndex = ItDisplay->second;
    }
    else
    {
        DisplayIndex = DisplayStringPool.size();
        DisplayStringPool.push_back(Str);
        DisplayMap[Str] = DisplayIndex;
    }

    return { ComparisonIndex, DisplayIndex };
}

FName FNameTable::GetUniqueName(const FString& BaseStr)
{
    TPair<int32, int32> Indices = FindOrAddName(BaseStr);
    int32 ComparisonIndex = Indices.first;
    int32 DisplayIndex = Indices.second;

    int32 Number = NextNumberMap[BaseStr];
    NextNumberMap[BaseStr]++;

    return FName(ComparisonIndex, DisplayIndex, Number);
}

FString FNameTable::GetDisplayString(int32 Idx) const
{
    if (Idx >= 0 && Idx < DisplayStringPool.size())
    {
        return DisplayStringPool[Idx];
    }
    static const FString EmptyString = "None";
    return EmptyString;
}

FString FNameTable::ToLower(const FString& Str)
{
    FString LowerStr = Str;
    std::transform(LowerStr.begin(), LowerStr.end(), LowerStr.begin(),
        [](unsigned char C) { return std::tolower(C); });
    return LowerStr;
}