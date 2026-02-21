#pragma once
#include "pch.h"

/**
 * @class FSkinningStats
 * @brief CPU/GPU 스키닝 성능 통계를 추적하고 관리하는 싱글톤 클래스
 *
 * 이동 평균(Moving Average)을 사용하여 프레임 간 변동을 부드럽게 처리합니다.
 */
class FSkinningStats
{
public:
	/**
	 * @brief 싱글톤 인스턴스 가져오기
	 */
	static FSkinningStats& GetInstance()
	{
		static FSkinningStats Instance;
		return Instance;
	}

	/**
	 * @brief CPU 스키닝 시간 기록 (밀리초)
	 */
	void RecordCPUSkinningTime(double TimeMS)
	{
		CPUSkinningTimes[CPUSampleIndex] = TimeMS;
		CPUSampleIndex = (CPUSampleIndex + 1) % MaxSamples;
		CPUSampleCount = std::min(CPUSampleCount + 1, MaxSamples);
	}

	/**
	 * @brief GPU 스키닝 시간 기록 (밀리초)
	 */
	void RecordGPUSkinningTime(double TimeMS)
	{
		GPUSkinningTimes[GPUSampleIndex] = TimeMS;
		GPUSampleIndex = (GPUSampleIndex + 1) % MaxSamples;
		GPUSampleCount = std::min(GPUSampleCount + 1, MaxSamples);
	}

	/**
	 * @brief 평균 CPU 스키닝 시간 계산 (밀리초)
	 */
	double GetAverageCPUSkinningTime() const
	{
		if (CPUSampleCount == 0) return 0.0;

		double Sum = 0.0;
		for (uint32 i = 0; i < CPUSampleCount; ++i)
		{
			Sum += CPUSkinningTimes[i];
		}
		return Sum / static_cast<double>(CPUSampleCount);
	}

	/**
	 * @brief 평균 GPU 스키닝 시간 계산 (밀리초)
	 */
	double GetAverageGPUSkinningTime() const
	{
		if (GPUSampleCount == 0) return 0.0;

		double Sum = 0.0;
		for (uint32 i = 0; i < GPUSampleCount; ++i)
		{
			Sum += GPUSkinningTimes[i];
		}
		return Sum / static_cast<double>(GPUSampleCount);
	}

	/**
	 * @brief 최근 CPU 스키닝 시간 (밀리초) - 실시간 표시용
	 */
	double GetLastCPUSkinningTime() const
	{
		if (CPUSampleCount == 0) return 0.0;
		uint32 LastIndex = (CPUSampleIndex + MaxSamples - 1) % MaxSamples;
		return CPUSkinningTimes[LastIndex];
	}

	/**
	 * @brief 최근 GPU 스키닝 시간 (밀리초) - 실시간 표시용
	 */
	double GetLastGPUSkinningTime() const
	{
		if (GPUSampleCount == 0) return 0.0;
		uint32 LastIndex = (GPUSampleIndex + MaxSamples - 1) % MaxSamples;
		return GPUSkinningTimes[LastIndex];
	}

	/**
	 * @brief 통계 초기화
	 */
	void Reset()
	{
		CPUSampleIndex = 0;
		CPUSampleCount = 0;
		GPUSampleIndex = 0;
		GPUSampleCount = 0;

		for (uint32 i = 0; i < MaxSamples; ++i)
		{
			CPUSkinningTimes[i] = 0.0;
			GPUSkinningTimes[i] = 0.0;
		}
	}

private:
	FSkinningStats()
	{
		Reset();
	}

	~FSkinningStats() = default;
	FSkinningStats(const FSkinningStats&) = delete;
	FSkinningStats& operator=(const FSkinningStats&) = delete;

	// Moving average를 위한 샘플 버퍼 (60 프레임 = 1초 @ 60 FPS)
	static constexpr uint32 MaxSamples = 60;

	double CPUSkinningTimes[MaxSamples];
	uint32 CPUSampleIndex = 0;
	uint32 CPUSampleCount = 0;

	double GPUSkinningTimes[MaxSamples];
	uint32 GPUSampleIndex = 0;
	uint32 GPUSampleCount = 0;
};
