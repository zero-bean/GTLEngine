#pragma once
#include "UEContainer.h"

// 파티클 시스템 통계 구조체
// 씬에 존재하는 파티클 시스템들의 통계를 추적
struct FParticleStats
{
	// 파티클 시스템 개수
	uint32 TotalParticleSystems = 0;

	// 이미터 개수
	uint32 TotalEmitters = 0;

	// 파티클 개수
	uint32 TotalActiveParticles = 0;
	uint32 TotalMaxParticles = 0;  // 최대 허용 파티클 수
	uint32 PeakParticles = 0;       // 피크 파티클 수

	// 메모리 사용량 (바이트 단위) - 실제 활성 파티클 기준
	uint64 ActiveParticleDataMemory = 0;   // 활성 파티클 데이터 메모리
	uint64 InstanceDataMemory = 0;         // 인스턴스 데이터 메모리
	uint64 TotalMemoryBytes = 0;           // 총 메모리 사용량

	// 모든 통계를 0으로 리셋
	void Reset()
	{
		TotalParticleSystems = 0;
		TotalEmitters = 0;
		TotalActiveParticles = 0;
		TotalMaxParticles = 0;
		PeakParticles = 0;
		ActiveParticleDataMemory = 0;
		InstanceDataMemory = 0;
		TotalMemoryBytes = 0;
	}

	// 피크 파티클 수 업데이트
	void UpdatePeak()
	{
		if (TotalActiveParticles > PeakParticles)
		{
			PeakParticles = TotalActiveParticles;
		}
	}

	// 총 메모리 계산
	void CalculateTotalMemory()
	{
		TotalMemoryBytes = ActiveParticleDataMemory + InstanceDataMemory;
	}

	// 메모리를 MB 단위로 반환
	double GetTotalMemoryMB() const
	{
		return static_cast<double>(TotalMemoryBytes) / (1024.0 * 1024.0);
	}
};

// 파티클 통계 전역 매니저 (싱글톤)
// UStatsOverlayD2D에서 접근할 수 있도록 전역 통계 제공
class FParticleStatManager
{
public:
	static FParticleStatManager& GetInstance()
	{
		static FParticleStatManager Instance;
		return Instance;
	}

	// 통계 업데이트
	void UpdateStats(const FParticleStats& InStats)
	{
		CurrentStats = InStats;
		CurrentStats.UpdatePeak();
	}

	// 통계 조회
	const FParticleStats& GetStats() const
	{
		return CurrentStats;
	}

	// 통계 리셋
	void ResetStats()
	{
		CurrentStats.Reset();
	}

	// 피크 리셋
	void ResetPeak()
	{
		CurrentStats.PeakParticles = 0;
	}

private:
	FParticleStatManager() = default;
	~FParticleStatManager() = default;
	FParticleStatManager(const FParticleStatManager&) = delete;
	FParticleStatManager& operator=(const FParticleStatManager&) = delete;

	FParticleStats CurrentStats;
};
