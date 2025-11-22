#include "pch.h"
#include "ParticleEmitterInstances.h"
#include "ParticleHelper.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleEmitter.h"

FParticleEmitterInstance::FParticleEmitterInstance(UParticleSystemComponent* InComponent)
	:OwnerComponent(InComponent)
{
}

void FParticleEmitterInstance::InitParticles()
{
	if (!SpriteTemplate)
	{
		return;
	}
	UParticleLODLevel* HightestLODLevel = SpriteTemplate->LODLevels[0];

	CurrentMaterial = HightestLODLevel->RequiredModule->Material.Get();

	// 이미터는 계속해서 상태를 초기화하고 이미팅을 반복함. 매번 ParticleEmiiter로부터 메모리 레이아웃을 재설정 하고 인스턴스 데이터 realloc할 필요 없이
	// 상태만 초기화해주면 됨. 그래서 처음 이니셜라이즈 되는 경우만 메모리 레이아웃 설정
	if (ParticleSize == 0)
	{

	}
}

// StartTime: 현재 프레임에서 첫 번째 파티클이 태어날 때까지 프레임 단위 시간 + 전 프레임에서 넘겨받은 시간
// Increment: 파티클이 생성 시간 간격(1초에 n개인 경우 1/n초)
// 위 두 시간을 조합해서 프레임 상관 없이 1초에 n개의 파티클 생성 가능.
void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity)
{
	if (Count <= 0)
		return;

	UParticleLODLevel* HightestLODLevel = SpriteTemplate->LODLevels[0];
	uint32 AvailableCount = MaxActiveParticles - ActiveParticles;
	uint32 SpawnCount = Count <= AvailableCount ? Count : AvailableCount;

	for (int32 Index = 0; Index < SpawnCount; Index++)
	{
		uint32 NewParticleIndex = ActiveParticles;

		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * NewParticleIndex)

		PreSpawn(Particle, InitialLocation, InitialVelocity);

		for (int32 ModuleIndex = 0; ModuleIndex < CurrentLODLevel->SpawnModules.Num(); ModuleIndex++)
		{
			UParticleModule* SpawnModule = CurrentLODLevel->SpawnModules[ModuleIndex];
			if (SpawnModule->bEnabled)
			{
				// LOD레벨마다 독자적인 메모리를 관리하는 게 아니라 ParticleData를 공유함. 그래서
				// LOD레벨이 다를 때 LOD 레벨 0에서의 모듈 오프셋을 알아야 초기화가 가능함.
				// 그래서 무조건 LOD 레벨 0의 메모리 레이아웃을 따라가도록 함. 
				UParticleModule* OffsetModule = HightestLODLevel->SpawnModules[ModuleIndex];
				SpawnModule->Spawn(FSpawnContext(this, GetModuleOffset(OffsetModule), StartTime, Particle));
			}
		}

		// StartTime에 태어난 파티클이 가장 오래된 파티클 => 가장 많이 이동 
		// 그 뒤에 태어난 파티클은 Increment만큼 뒤에 태어난 파티클이므로 Increment시간만큼 덜 이동한 것임
		// 2번째 인자는 이미터가 이동한 거리까지 고려한 것인데 일단 정적 이미터만 처리하기로 함.
		PostSpawn(Particle, 0, StartTime - Increment * Index);
	}
}

void FParticleEmitterInstance::PostSpawn(FBaseParticle* InParticle, float InterpolationPercentage, float SpawnTime)
{

	// 태어난 시간 만큼 이동
	InParticle->Location += InParticle->Velocity * SpawnTime;
	InParticle->RelativeTime = SpawnTime;
}

uint32 FParticleEmitterInstance::GetModuleOffset(UParticleModule* InModule)
{
	if (!InModule)
		return 0;

	uint32* Offset = SpriteTemplate->ModuleOffsetMap.Find(InModule);
	return (Offset) ? *Offset : 0;
}

// 파티클 하나 초기화 하는 함수
void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity)
{
	if (!Particle)
	{
		return;
	}

	// 다른 파티클이 쓰던 값이 들어있을 수 있으므로 0으로 초기화
	// ParticleSize만큼 해야함(페이로드 있을 수 있음)
	std::memset(Particle, 0, ParticleSize);

	Particle->Location = InitialLocation;
	Particle->BaseVelocity = InitialVelocity;
	Particle->Velocity = InitialVelocity;
	Particle->RelativeTime = 0;

}

void FParticleDataContainer::Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts)
{
	if (!(InParticleDataNumBytes > 0 && InParticleIndicesNumShorts >= 0 &&
		// 인덱스가 uint16이라서 인덱스 시작 주소가 짝수 주소에서 시작해야함. 그래서 앞의 데이터바이트 크기가 짝수인지 확인함
		(InParticleDataNumBytes & sizeof(uint16)) == 0))
	{
		return;
	}
	ParticleDataNumBytes = InParticleDataNumBytes;
	ParticleIndicesNumShorts = InParticleIndicesNumShorts;

	MemBlockSize = ParticleDataNumBytes + ParticleIndicesNumShorts * sizeof(uint16);

	ParticleData = (uint8*)std::malloc(MemBlockSize);
	std::memset(ParticleData, 0, MemBlockSize);
	ParticleIndices = (uint16*)(ParticleData + ParticleDataNumBytes);
}
void FParticleDataContainer::Free()
{
	if (ParticleData)
	{
		std::free(ParticleData);
		ParticleData = nullptr;
	}
}

FParticleSpriteEmitterInstance::FParticleSpriteEmitterInstance(UParticleSystemComponent* InComponent)
	:FParticleEmitterInstance(InComponent)
{
}

// bSelected: 선택된 이미터인 경우 에디터에서 바운딩 박스 표시하기 위함
FDynamicEmitterDataBase* FParticleSpriteEmitterInstance::GetDynamicData(bool bSelected)
{
	return nullptr;
}
