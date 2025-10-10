#include "pch.h"
#include "Core/Public/ScopeCycleCounter.h"

TMap<FString, FTimeProfile> TimeProfileMap;
void FScopeCycleCounter::AddTimeProfile(const TStatId StatId, const double Milliseconds)
{	if (TimeProfileMap.find(StatId.Key) != TimeProfileMap.end())
	{
		TimeProfileMap[StatId.Key] = FTimeProfile{Milliseconds, 1};
	}
	else
	{
		TimeProfileMap[StatId.Key].Milliseconds += Milliseconds;
		TimeProfileMap[StatId.Key].CallCount++;
	}
}

void FScopeCycleCounter::TimeProfileInit()
{
	const TArray<FString> Keys = TimeProfileMap.GetKeys();
	for (const FString& Key : Keys)
	{
		TimeProfileMap[Key].Milliseconds = 0;
		TimeProfileMap[Key].CallCount = 0;
	}
}

const TArray<FString> FScopeCycleCounter::GetTimeProfileKeys()
{
	return TimeProfileMap.GetKeys();
}

const TArray<FTimeProfile> FScopeCycleCounter::GetTimeProfileValues()
{
	return TimeProfileMap.GetValues();
}

const FTimeProfile& FScopeCycleCounter::GetTimeProfile(const FString& Key)
{
	return TimeProfileMap[Key];
}
