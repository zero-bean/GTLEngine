#pragma once
#include "MeshBatchElement.h"
#include "ParticleHelper.h"
class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
class UParticleModule;
class UParticleModuleTypeDataBeam;
struct FBaseParticle;
struct FDynamicEmitterDataBase;
struct FBeamParticleInstance;

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

	uint32 CurrentActiveParticleCapacity = 0;

	// 파티클 ID로 씀. GUObjectArray의 InternalIndex같은 느낌.
	uint32 ParticleCounter = 0;

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
	virtual ~FParticleEmitterInstance();


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

	virtual void Init();

	virtual void Tick(float DeltaTime, bool bSuppressSpawning);

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

	// 이미터의 정규화된 시간(0.0~1.0) 반환 - 커브 샘플링용
	float GetNormalizedEmitterTime() const;
};

struct FParticleSpriteEmitterInstance : FParticleEmitterInstance
{
public:

	UQuad* Quad = nullptr;

	TArray<FSpriteParticleInstance> ParticleInstanceData;
	
	// ParticleSystemComponent 헤더가 방대해질 가능성이 높음, 인터페이스를 만들어야 함.
	FParticleSpriteEmitterInstance(UParticleSystemComponent* InComponent);
	~FParticleSpriteEmitterInstance() override {};
	void Tick(float DeltaTime, bool bSuppressSpawning) override;

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
	~FParticleMeshEmitterInstance() override {};
	void Tick(float DeltaTime, bool bSuppressSpawning) override;

	void SetMeshMaterials(TArray<UMaterialInterface*>& MeshMaterials) override;
	void FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) override;
};

struct FParticleBeamEmitterInstance : FParticleEmitterInstance
{
	struct FBeamPoint
	{
		// 보간된 위치
		FVector Position;
		// taper가 적용된 두께
		float Width;
		// 길이 비율에 따른 U 좌표
		float TexCoord;
		float Progress;
		FVector4 Color;
	};
public:
	// 캐싱용 변수
	// FParticleEmitterInstance의 CurrentLODLevel에서 TypeDataModule을 매번 Casting하지 않고
	// 처음 한 번 캐스팅 해서 사용
	UParticleModuleTypeDataBeam* BeamTypeData;

	// 액터가 존재하면 액터의 위치로 갱신
	FVector SourcePosition;
	FVector TargetPosition;

	// 휘어지는 경우 필요, 베지어 곡선 적용에 필요함
	// 접선 : 진행 방향, 베지어 곡선의 제어점 역할
	FVector SourceTangent;
	FVector TargetTangent;	

	// 액터 캐싱용 변수
	AActor* SourceActor = nullptr;
	
	AActor* TargetActor = nullptr;

	int32 ActiveBeamCount;

	// 현재 빔의 길이 상태
	float CurrentTravelDistance;

	TArray<FBeamPoint> BeamPoints;

	float BaseWidth;

	TArray<FBeamParticleInstance> RenderVertices;

	float BeamLength;
	
	FParticleBeamEmitterInstance(UParticleSystemComponent* InComponent);
	~FParticleBeamEmitterInstance() override {};

	void GetParticleInstanceData(TArray<FBeamParticleInstance>& ParticleInstanceData);

	void Init() override;

	void Tick(float DeltaTime, bool bSuppressSpawning) override;

	void BuildBeamPoints();

	void BuildBeamMesh(const FVector& CameraPosition);

	void FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View) override;
};