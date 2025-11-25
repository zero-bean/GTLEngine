#include "pch.h"
#include "ParticleEmitterInstances.h"
#include "ParticleHelper.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"
#include "ParticleModuleSpawn.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "ParticleModuleTypeDataMesh.h"

FParticleEmitterInstance::FParticleEmitterInstance(UParticleSystemComponent* InComponent)
	:OwnerComponent(InComponent)
{
}

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	if (ParticleData)
	{
		// malloc으로 만들어서 free -> 소멸자 호출 x
		// delete로 지우면 쓰레기 소멸자 호출 -> 터짐
		free(ParticleData);
		ParticleData = nullptr;
	}
	if (ParticleIndices)
	{
		free(ParticleIndices);
		ParticleIndices = nullptr;
	}
	if (InstanceData)
	{
		free(InstanceData);
		InstanceData = nullptr;
	}

	ActiveParticles = 0;
	MaxActiveParticles = 0;
}

void FParticleEmitterInstance::SetMeshMaterials(TArray<UMaterialInterface*>& MeshMaterials)
{
}
void FParticleEmitterInstance::Init()
{
	if (!SpriteTemplate)
	{
		return;
	}
	UParticleLODLevel* HightestLODLevel = SpriteTemplate->LODLevels[0];

	CurrentMaterial = HightestLODLevel->RequiredModule->Material;
	CurrentLODLevel = HightestLODLevel;

	bool bNeedsInit = (ParticleSize == 0);
	// 이미터는 계속해서 상태를 초기화하고 이미팅을 반복함. 매번 ParticleEmiiter로부터 메모리 레이아웃을 재설정 하고 인스턴스 데이터 realloc할 필요 없이
	// 상태만 초기화해주면 됨. 그래서 처음 이니셜라이즈 되는 경우만 메모리 레이아웃 설정
	if (bNeedsInit)
	{
		ParticleSize = SpriteTemplate->ParticleSize;

		if ((InstanceData == nullptr) || (SpriteTemplate->ReqInstanceBytes > InstancePayloadSize))
		{
			InstanceData = (uint8*)(std::realloc(InstanceData, SpriteTemplate->ReqInstanceBytes));
			InstancePayloadSize = SpriteTemplate->ReqInstanceBytes;

		}

		std::memset(InstanceData, 0, InstancePayloadSize);

		// 인스턴스 데이터를 0이 아닌 다른 값으로 초기화 해야하는 모듈이 있을 수 있음
		for (UParticleModule* ParticleModule : SpriteTemplate->ModulesNeedingInstanceData)
		{
			uint8* PrepInstanceData = GetModuleInstanceData(ParticleModule);
			if (PrepInstanceData)
			{
				ParticleModule->PrepPerInstanceBlock(this, (void*)PrepInstanceData);
			}
		}

		PayloadOffset = ParticleSize;

		// 이미 ParticleEmitter에서 페이로드까지 포함한 사이즈 아닌가?
		// -> 직렬화 대상, UI 에디팅 대상이 아닌 페이로드 데이터를 써야하는 인스턴스가 존재한다고 함.
		ParticleSize += RequiredBytes();

		// 16바이트 정렬: 16배수인 경우 그대로, 아닌 경우 더 큰 16배수로 반올림
		ParticleSize = (ParticleSize + 15) & ~15;

		ParticleStride = ParticleSize;
	}

	// 메시 이미터 인스턴스 전용
	SetMeshMaterials(SpriteTemplate->MeshMaterials);

	if (ParticleData == nullptr)
	{
		MaxActiveParticles = 0;
		ActiveParticles = 0;
	}

	// 파이일때는 최적화를 위해 미리 파티클 메모리 할당. 
	// 에디터에서는 그냥 0부터 동적할당
	if (bNeedsInit && GWorld->bPie)
	{
		if ((HightestLODLevel->PeakActiveParticles > 0) || (SpriteTemplate->InitialAllocationCount > 0))
		{
			// 초기 메모리 최대 100개 제한
			if (SpriteTemplate->InitialAllocationCount > 0)
			{
				Resize(FMath::Min(SpriteTemplate->InitialAllocationCount, 100));
			}
			else
			{
				Resize(FMath::Min(HightestLODLevel->PeakActiveParticles, 100));
			}
		}
		else
		{
			Resize(10);
		}
	}
}

void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
	// 이미터 시간 갱신
	EmitterTime += DeltaTime;

	// 생명주기 끝난 파티클 제거
	KillParticles();

	// 파티클 스폰
	if (!bSuppressSpawning)
	{
		int32 SpawnCount = CurrentLODLevel->SpawnModule->GetSpawnCount(DeltaTime, SpawnFraction, EmitterTime);

		if (SpawnCount > 0)
		{
			// ActiveParticles는 실제로 Spawn할때 올림
			Resize(ActiveParticles + SpawnCount);
			SpawnParticles(SpawnCount, DeltaTime, DeltaTime / SpawnCount, OwnerComponent->GetWorldLocation(), FVector(0, 0, 0));

		}
	}

	// 파티클 업데이트
	UpdateParticles(DeltaTime);

	// 이미터 생명주기 파악
	float EmitterDuration = CurrentLODLevel->RequiredModule->EmitterDuration;

	if (EmitterDuration > 0.0f)
	{
		if (EmitterTime >= EmitterDuration)
		{
			if ((CurrentLODLevel->RequiredModule->EmitterLoops == 0) ||
				(LoopCount < CurrentLODLevel->RequiredModule->EmitterLoops))
			{
				EmitterTime -= EmitterDuration;

				LoopCount++;
			}
			else
			{
				bEmitterIsDone = true;
			}
		}
	}

}
void FParticleEmitterInstance::UpdateParticles(float DeltaTime)
{

	// 파티클 나이 먹이기(메모리 관리는 KillParticles에서 함)
	BEGIN_UPDATE_LOOP
		Particle.RelativeTime += DeltaTime * Particle.OneOverMaxLiftTime;
	END_UPDATE_LOOP

	// 모듈 업데이트
	for (UParticleModule* ParticleModule : CurrentLODLevel->UpdateModules)
	{
		ParticleModule->Update(FUpdateContext(this, GetModuleOffset(ParticleModule), DeltaTime));
	}

	//위치 업데이트
	BEGIN_UPDATE_LOOP
		Particle.Location += Particle.Velocity * DeltaTime;
		Particle.Rotation += Particle.RotationRate * DeltaTime;
	END_UPDATE_LOOP
}
void FParticleEmitterInstance::KillParticles()
{
	int32 Index = 0;
	BEGIN_UPDATE_LOOP
		if (Particle.RelativeTime >= 1.0f)
		{
			int32 LastSlotIndex = ActiveParticles - 1;
			if (Index != LastSlotIndex)
			{
				ParticleIndices[Index] = ParticleIndices[LastSlotIndex];
				ParticleIndices[LastSlotIndex] = CurrentIndex;
			}
			ActiveParticles--;
		}
	END_UPDATE_LOOP
}


// NewMaxActiveParticles만큼 ParticleData 메모리 Resize
// bSetMaxActiveCount: HightestLODLevel의 PeakActiveParticles를 위의 인자로 업데이트
bool FParticleEmitterInstance::Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount)
{

	if (NewMaxActiveParticles > MaxActiveParticles)
	{
		ParticleData = (uint8*)std::realloc(ParticleData, ParticleStride * NewMaxActiveParticles);

		if (ParticleIndices == nullptr)
		{
			// 초기 할당일 경우 인덱스 처음부터 새로 할당
			MaxActiveParticles = 0;
		}
		// 왜 +1? => 소팅할때 임시버퍼로 필요함
		ParticleIndices = (uint16*)std::realloc(ParticleIndices, sizeof(uint16) * (NewMaxActiveParticles + 1));


		for (int32 Index = MaxActiveParticles; Index < NewMaxActiveParticles; Index++)
		{
			ParticleIndices[Index] = Index;
		}

		MaxActiveParticles = NewMaxActiveParticles;
	}

	if (bSetMaxActiveCount)
	{
		UParticleLODLevel* HightestLODLevel = SpriteTemplate->LODLevels[0];

		if (HightestLODLevel)
		{
			if (MaxActiveParticles > HightestLODLevel->PeakActiveParticles)
			{
				HightestLODLevel->PeakActiveParticles = MaxActiveParticles;
			}
		}
	}

	return true;
}

bool FParticleEmitterInstance::HasComplete()
{
	if (SpriteTemplate == nullptr)
	{
		return true;
	}

	if (!bEmitterIsDone)
	{
		return false;
	}

	if (ActiveParticles > 0)
	{
		return false;
	}

	return true;
}

// StartTime: 현재 프레임에서 첫 번째 파티클이 태어날 때까지 프레임 단위 시간 + 전 프레임에서 넘겨받은 시간, 하지만 정신건강을 위해 그냥 DeltaTime을 씀
// Increment: 파티클의 생성 시간 간격(1초에 n개인 경우 1/n초), 하지만 정신건강을 위해 (이번 프레임 DeltaTime) / (이번 프레임 SpawnCount) 값을 씀.)
// 진정한 시뮬레이션을 위해서는 StartTime과 Increment를 제대로 계산해야함(ex. FPS에서의 총알)
// InitialLocation: 이미터 로케이션
// InitialVelocity: 이미터 속도
void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity)
{
	if (Count <= 0)
		return;

	UParticleLODLevel* HightestLODLevel = SpriteTemplate->LODLevels[0];
	uint32 AvailableCount = MaxActiveParticles - ActiveParticles;
	uint32 SpawnCount = Count <= AvailableCount ? Count : AvailableCount;

	for (int32 Index = 0; Index < SpawnCount; Index++)
	{
		uint32 NewParticleIndex = ParticleIndices[ActiveParticles];

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
		ActiveParticles++;
	}
	
}

void FParticleEmitterInstance::PostSpawn(FBaseParticle* InParticle, float InterpolationPercentage, float SpawnTime)
{

	// 태어난 시간 만큼 이동
	InParticle->Location += InParticle->Velocity * SpawnTime;
	InParticle->RelativeTime = SpawnTime;
}

uint32 FParticleEmitterInstance::RequiredBytes()
{
	return 0;
}

uint32 FParticleEmitterInstance::GetModuleOffset(UParticleModule* InModule)
{
	if (!InModule)
		return 0;

	uint32* Offset = SpriteTemplate->ModuleOffsetMap.Find(InModule);
	return (Offset) ? *Offset : 0;
}

uint8* FParticleEmitterInstance::GetModuleInstanceData(UParticleModule* InModule)
{
	if (InstanceData)
	{
		uint32* Offset = SpriteTemplate->ModuleInstanceOffsetMap.Find(InModule);

		if (Offset)
		{
			if (InstancePayloadSize > *Offset)
			{
				return &(InstanceData[*Offset]);
			}
		}
	}
	return nullptr;
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

void FParticleEmitterInstance::GetParticleInstanceData(TArray<FSpriteParticleInstance>& ParticleInstanceData)
{
	BEGIN_UPDATE_LOOP
		FSpriteParticleInstance NewInstance;
	NewInstance.Color = FVector4(Particle.Color.R, Particle.Color.G, Particle.Color.B, 1.0f);

	NewInstance.LifeTime = Particle.Lifetime;
	NewInstance.Position = Particle.Location;
	NewInstance.Rotation = DegreesToRadians(Particle.Rotation);
	NewInstance.Size = Particle.Size.X;

	ParticleInstanceData.Add(NewInstance);
	END_UPDATE_LOOP
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
	Quad = UResourceManager::GetInstance().Get<UQuad>("BillboardQuad");
}


void FParticleSpriteEmitterInstance::FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View)
{
	FMeshBatchElement BatchElement;

	ParticleInstanceData.Empty();
	GetParticleInstanceData(ParticleInstanceData);
	BatchElement.BlendType = EBlendType::Alpha;
	BatchElement.BaseVertexIndex = 0;
	BatchElement.Distance = (View->ViewLocation - EmitterLocation * OwnerComponent->GetWorldMatrix()).Size();

	BatchElement.VertexBuffer = Quad->GetVertexBuffer();
	BatchElement.VertexStride = sizeof(FBillboardVertex);
	BatchElement.IndexBuffer = Quad->GetIndexBuffer();
	BatchElement.IndexCount = 6;
	BatchElement.ParticleInstanceData = &ParticleInstanceData;
	BatchElement.InstanceStride = sizeof(FSpriteParticleInstance);
	BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	BatchElement.InstanceShaderResourceView = InstanceSRV;
	
	if (CurrentMaterial)
	{
		BatchElement.Material = CurrentMaterial;
		UShader* Shader = UResourceManager::GetInstance().Load<UShader>("Shaders/Particles/SpriteParticle.hlsl");

		FShaderVariant* ShaderVariant = Shader->GetOrCompileShaderVariant();
		if (ShaderVariant)
		{
			BatchElement.VertexShader = ShaderVariant->VertexShader;
			BatchElement.PixelShader = ShaderVariant->PixelShader;
			BatchElement.InputLayout = ShaderVariant->InputLayout;
		}
	}
	MeshBatch.Add(BatchElement);
}

FParticleMeshEmitterInstance::FParticleMeshEmitterInstance(UParticleSystemComponent* InComponent)
	:FParticleEmitterInstance(InComponent)
{
}

void FParticleMeshEmitterInstance::SetMeshMaterials(TArray<UMaterialInterface*>& MeshMaterials)
{
	CurrentMaterials = MeshMaterials;
}

void FParticleMeshEmitterInstance::FillMeshBatch(TArray<FMeshBatchElement>& MeshBatch, const FSceneView* View)
{
	
	UStaticMesh* StaticMesh = static_cast<UParticleModuleTypeDataMesh*>(CurrentLODLevel->TypeDataModule)->StaticMesh;
	if (!StaticMesh || !StaticMesh->GetStaticMeshAsset())
	{
		return;
	}
	FMeshBatchElement BatchElement;
	BatchElement.BlendType = EBlendType::Opaque;
	BatchElement.Distance = (View->ViewLocation - EmitterLocation * OwnerComponent->GetWorldMatrix()).Size();

	ParticleInstanceData.Empty();
	GetParticleInstanceData(ParticleInstanceData);

	BatchElement.ParticleInstanceData = &ParticleInstanceData;  
	BatchElement.InstanceStride = sizeof(FSpriteParticleInstance);

	const TArray<FGroupInfo>& MeshGroupInfos = StaticMesh->GetMeshGroupInfo();
	const bool bHasSections = !MeshGroupInfos.IsEmpty();
	const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;

	UShader* Shader = UResourceManager::GetInstance().Load<UShader>("Shaders/Particles/SpriteParticle.hlsl");
	TArray<FShaderMacro> ShaderMacros;
	ShaderMacros.Add(FShaderMacro("MESH_PARTICLE", "1"));
	FShaderVariant* ShaderVariant = Shader->GetOrCompileShaderVariant(ShaderMacros);

	
	for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
	{
		uint32 IndexCount = 0;
		uint32 StartIndex = 0;
		UMaterialInterface* Material = nullptr;
		if (bHasSections)
		{
			const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
			Material = UResourceManager::GetInstance().Load<UMaterial>(Group.InitialMaterialName);
			IndexCount = Group.IndexCount;
			StartIndex = Group.StartIndex;
		}
		else
		{
			IndexCount = StaticMesh->GetIndexCount();
			StartIndex = 0;
		}

		if (IndexCount == 0)
		{
			continue;
		}

		if (CurrentMaterials.Num() > 0 && bMaterialOverride)
		{
			BatchElement.Material = CurrentMaterials[SectionIndex];
		}
	
		if (ShaderVariant)
		{
			BatchElement.VertexShader = ShaderVariant->VertexShader;
			BatchElement.PixelShader = ShaderVariant->PixelShader;
			BatchElement.InputLayout = ShaderVariant->InputLayout;
		}

		// UMaterialInterface를 UMaterial로 캐스팅해야 할 수 있음. 렌더러가 UMaterial을 기대한다면.
		// 지금은 Material.h 구조상 UMaterialInterface에 필요한 정보가 다 있음.
		BatchElement.Material = Material;
		BatchElement.VertexBuffer = StaticMesh->GetVertexBuffer();
		BatchElement.IndexBuffer = StaticMesh->GetIndexBuffer();
		BatchElement.VertexStride = StaticMesh->GetVertexStride();
		BatchElement.IndexCount = IndexCount;
		BatchElement.StartIndex = StartIndex;
		BatchElement.BaseVertexIndex = 0;
		BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		MeshBatch.Add(BatchElement);
	}
}
