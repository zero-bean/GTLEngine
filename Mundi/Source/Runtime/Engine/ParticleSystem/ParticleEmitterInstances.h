#pragma once
#include "MeshBatchElement.h"
#include "ParticleHelper.h"
class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
class UParticleModule;
struct FBaseParticle;

struct FParticleSortKey
{
	uint16 Index;
	float SortKey;
};

struct FParticleEmitterInstance
{
public:

	UParticleEmitter* SpriteTemplate = nullptr;

	UParticleSystemComponent* OwnerComponent = nullptr;

	UParticleLODLevel* CurrentLODLevel = nullptr;

	int32 CurrentLODLevelIndex = 0;

	uint8* ParticleData = nullptr;

	uint16* ParticleIndices = nullptr;

	// 현재 인스턴스 생성 이후 얼마나 지났는지 등 인스턴스 상태 저장.(모듈들이 이 인스턴스 데이터를 파티클처럼 쪼개서 각각 본인의 상태를 저장함)
	uint8* InstanceData = nullptr;

	int32 InstancePayloadSize = 0;

	int32 PayloadOffset = 0;

	// 파티클 하나의 바이트 단위 사이즈
	int32 ParticleSize = 0;

	// ParticleData Array에서 파티클 사이 Stride
	uint32 ParticleStride = 0;

	// 현재 활성화된 파티클 개수
	uint32 ActiveParticles = 0;

	// 파티클 ID로 씀. GUObjectArray의 InternalIndex같은 느낌.
	uint32 ParticleCounter = 0;

	// ActiveParticles의 최대 개수
	uint32 MaxActiveParticles = 0;

	// 초당 10마리 생성 -> 프레임당 0.16마리 생성 -> 생성 안됨 -> 이전 프레임 남은 시간 누적 필요
	float SpawnFraction = 0.0f;

	// 현재 이미터 루프카운트
	int32 LoopCount = 0;

	UMaterialInterface* CurrentMaterial = nullptr;

	// 컴포넌트 로컬 좌표계에서 정의되는 이미터 좌표
	FVector EmitterLocation = FVector::Zero();

	// 테스트용 텍스처
	ID3D11ShaderResourceView* InstanceSRV = nullptr;

	// 파티클 거리순 정렬 위한 인덱스 배열
	TArray<FParticleSortKey> ParticleIndicesForSort;

	float EmitterTime = 0.0f;

	// 이미터 루프 다 끝났는지 확인(끝나도 파티클이 남아있을 수 있음, 소멸 여부는 HasComplete함수가 결정)
	bool bEmitterIsDone = false;

	FParticleEmitterInstance(UParticleSystemComponent* InComponent);
	~FParticleEmitterInstance();


	template<typename KeyGetterFunction>
	void GenerateSortKey(TArray<FParticleSortKey>& ParticleIndicesForSort, KeyGetterFunction KeyGetter)
	{
		BEGIN_UPDATE_LOOP
			FParticleSortKey Key;
		Key.SortKey = KeyGetter(Particle);
		Key.Index = CurrentIndex;
		ParticleIndicesForSort.Add(Key);
		END_UPDATE_LOOP
	}

	virtual void SetMeshMaterials(TArray<UMaterialInterface*>& MeshMaterials);
	virtual void GetParticleInstanceData(TArray<FSpriteParticleInstance>& ParticleInstanceData, const FSceneView* View);
	virtual void FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) = 0;

	void Init();

	void Tick(float DeltaTime, bool bSuppressSpawning);

	void UpdateParticles(float DeltaTime);
	
	void KillParticles();

	bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = false);

	bool HasComplete();

	void SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity);

	void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity);

	void PostSpawn(FBaseParticle* InParticle, float InterpolationPercentage, float SpawnTime);

	uint32 RequiredBytes();

	uint32 GetModuleOffset(UParticleModule* InModule);

	uint8* GetModuleInstanceData(UParticleModule* InModule);
};

struct FParticleSpriteEmitterInstance : FParticleEmitterInstance
{
public:

	UQuad* Quad = nullptr;

	TArray<FSpriteParticleInstance> ParticleInstanceData;
	
	// ParticleSystemComponent 헤더가 방대해질 가능성이 높음, 인터페이스를 만들어야 함.
	FParticleSpriteEmitterInstance(UParticleSystemComponent* InComponent);

	void FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) override;
};

struct FParticleMeshEmitterInstance : FParticleEmitterInstance
{
public:
	// true일 경우 스테틱 메시 자체적인 머티리얼 무시하고 설정한 머티리얼로 렌더링
	bool bMaterialOverride = false;

	TArray<FSpriteParticleInstance> ParticleInstanceData;

	// 메시 인스터스의 경우 다중 매터리얼 지원해야함.
	// CurrentMaterial은 기본값이고 SpriteEmitterInstance는 그대로 사용
	TArray<UMaterialInterface*> CurrentMaterials;

	FParticleMeshEmitterInstance(UParticleSystemComponent* InComponent);

	void SetMeshMaterials(TArray<UMaterialInterface*>& MeshMaterials) override;
	void FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) override;
};