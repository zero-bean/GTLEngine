#pragma once
#include "Global/Types.h"

/*
측정 원하는 cpp에

#include "Core/Public/ScopeCycleCounter.h"

원하는 Scope의 시작에 TIME_PROFILE((원하는 Key 마음대로 ){A})
원하는 Scope의 마지막에 TIME_PROFILE_END({A})

만약 마지막에 매크로 안 달면, Scope 내 시간이 자동 측정된다.
*/
#ifdef _DEBUG
#define TIME_PROFILE(Key)\
FScopeCycleCounter Key##Counter(#Key);
#else
#define TIME_PROFILE(Key)
#endif

#ifdef _DEVELOP
#define TIME_PROFILE_END(Key)\
Key##Counter.Finish();
#else
#define TIME_PROFILE_END(Key)
#endif

class FWindowsPlatformTime
{
public:
	static double GSecondsPerCycle; // 0
	static bool bInitialized; // false

	static void InitTiming()
	{
		if (!bInitialized)
		{
			bInitialized = true;

			double Frequency = (double)GetFrequency();
			if (Frequency <= 0.0)
			{
				Frequency = 1.0;
			}

			GSecondsPerCycle = 1.0 / Frequency;
		}
	}
	static float GetSecondsPerCycle()
	{
		if (!bInitialized)
		{
			InitTiming();
		}
		return (float)GSecondsPerCycle;
	}
	static uint64 GetFrequency()
	{
		LARGE_INTEGER Frequency;
		QueryPerformanceFrequency(&Frequency);
		return Frequency.QuadPart;
	}
	static double ToMilliseconds(uint64 CycleDiff)
	{
		double Ms = static_cast<double>(CycleDiff)
			* GetSecondsPerCycle()
			* 1000.0;

		return Ms;
	}

	static uint64 Cycles64()
	{
		LARGE_INTEGER CycleCount;
		QueryPerformanceCounter(&CycleCount);
		return (uint64)CycleCount.QuadPart;
	}
};
