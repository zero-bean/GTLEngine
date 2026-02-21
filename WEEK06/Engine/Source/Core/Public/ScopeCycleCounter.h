#pragma once
#include "Core/Public/PlatformTime.h"

struct TStatId
{
	FString Key;
	TStatId() = default;
	TStatId(const FString& InKey) : Key(InKey) {}
};
struct FTimeProfile
{
	double Milliseconds;
	uint32 CallCount;

	const char* GetConstChar() const
	{
		static char buffer[64];
		snprintf(buffer, sizeof(buffer), " : %.3fms, Call : %d", Milliseconds, static_cast<int>(CallCount));
		return buffer;
	}
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
	FScopeCycleCounter()
		: StartCycles(FPlatformTime::Cycles64())
		, UsedStatId(TStatId{})
	{
	}

	FScopeCycleCounter(TStatId StatId)
		: StartCycles(FPlatformTime::Cycles64())
		, UsedStatId(StatId)
	{
	}

	FScopeCycleCounter(const FString& Key)
		: StartCycles(FPlatformTime::Cycles64())
		, UsedStatId(TStatId(Key))
	{
	}

	FScopeCycleCounter(const char* Key)
		: FScopeCycleCounter(FString(Key))
	{
	}

	~FScopeCycleCounter()
	{
		Finish();
	}

	double Finish()
	{
		if (bIsFinish == true)
		{
			return 0;
		}
		bIsFinish = true;
		const uint64 EndCycles = FPlatformTime::Cycles64();
		const uint64 CycleDiff = EndCycles - StartCycles;

		double Milliseconds = FWindowsPlatformTime::ToMilliseconds(CycleDiff);
		if (UsedStatId.Key.empty() == false)
		{
			AddTimeProfile(UsedStatId, Milliseconds);
		}
		return Milliseconds;
	}

	static void AddTimeProfile(const TStatId StatId, const double Milliseconds);
	static void TimeProfileInit();

	static const TArray<FString> GetTimeProfileKeys();
	static const TArray<FTimeProfile> GetTimeProfileValues();
	static const FTimeProfile& GetTimeProfile(const FString& Key);

private:
	bool bIsFinish = false;
	uint64 StartCycles;
	TStatId UsedStatId;
};
