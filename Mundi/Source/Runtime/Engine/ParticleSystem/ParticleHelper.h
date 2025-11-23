#pragma once

struct FBaseParticle
{
	FVector    Location;
	// 파티클의 현재 속도
	FVector    Velocity;
	float      RelativeTime;
	float      Lifetime;
	// 파티클이 원래 가지던 오리지널 속도(파티클 현재 속도를 2배로 하면 중력도 2배 적용)
	// 배속은 현재 속도가 아닌 BaseVelocity에 적용되어야 함.
	FVector    BaseVelocity;
	float      Rotation;
	float      RotationRate;
	FVector    Size;
	FLinearColor     Color;
	// 나눗셈 연산 비싸서 미리 구함.
	float OneOverMaxLiftTime;
};

// 모든 Particle에 대해 POD 오프셋 연산 똑같이 적용되므로 실수 방지를 위해 만든 매크로
#define DECLARE_PARTICLE_PTR(Name, Address)									\
	FBaseParticle* Name = (FBaseParticle*)(Address);

// 페이로드는 필요시 별도로 오프셋을 계산해서 수정해야함.
#define BEGIN_UPDATE_LOOP															\
for(int Index = ActiveParticles - 1 ; Index >= 0; Index--)							\
{																					\
	const int32 CurrentIndex = ParticleIndices[Index];								\
	const uint8* ParticlePtr = ParticleData + CurrentIndex * ParticleStride;		\
	FBaseParticle& Particle = *((FBaseParticle*) ParticlePtr);	

// DELCARE, BEGIN 이후에 Particle로 데이터 수정하고 END_UPDATE_LOOP써주면 알아서 오프셋 계산해서 모든 파티클 데이터 수정 가능하게 해줌
#define END_UPDATE_LOOP																\
}



// 렌더링 데이터 전달용. 실제 인스턴스가 쓰는 데이터는 ParticleData, ParticleIndices
struct FParticleDataContainer
{
	int32 MemBlockSize;
	int32 ParticleDataNumBytes;
	int32 ParticleIndicesNumShorts;
	uint8* ParticleData; // this is also the memory block we allocated
	uint16* ParticleIndices; // not allocated, this is at the end of the memory block

	FParticleDataContainer()
		: MemBlockSize(0)
		, ParticleDataNumBytes(0)
		, ParticleIndicesNumShorts(0)
		, ParticleData(nullptr)
		, ParticleIndices(nullptr)
	{
	}
	~FParticleDataContainer()
	{
		Free();
	}
	void Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts);
	void Free();
};
