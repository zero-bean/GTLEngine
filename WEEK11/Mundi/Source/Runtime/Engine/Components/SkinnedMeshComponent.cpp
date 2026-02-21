#include "pch.h"
#include "SkinnedMeshComponent.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "SkinningStats.h"

USkinnedMeshComponent::USkinnedMeshComponent() : SkeletalMesh(nullptr)
{
   bCanEverTick = true;
}

USkinnedMeshComponent::~USkinnedMeshComponent()
{
   if (VertexBuffer)
   {
      VertexBuffer->Release();
      VertexBuffer = nullptr;
   }

   // GPU 리소스 해제 (각 컴포넌트가 자신의 리소스 소유)
   // GPU Query는 CPU/GPU 모드 모두 사용하므로 항상 해제
   ReleaseGPUQueryResources();

   //// GPU 스키닝 버퍼는 GPU 모드일 때만 해제
   //if (bOwnsGPUResources)
   //{
      if (GPUSkinnedVertexBuffer)
      {
         GPUSkinnedVertexBuffer->Release();
         GPUSkinnedVertexBuffer = nullptr;
      }

      if (BoneMatrixConstantBuffer)
      {
         BoneMatrixConstantBuffer->Release();
         BoneMatrixConstantBuffer = nullptr;
      }
   //}
}

void USkinnedMeshComponent::BeginPlay()
{
   Super::BeginPlay();
}

void USkinnedMeshComponent::TickComponent(float DeltaTime)
{
   UMeshComponent::TickComponent(DeltaTime);
   ReadGPUQueryResults();
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
   Super::Serialize(bInIsLoading, InOutHandle);

   if (bInIsLoading)
   {
      SetSkeletalMesh(SkeletalMesh->GetPathFileName());
   }
   // @TODO - UStaticMeshComponent처럼 프로퍼티 기반 직렬화 로직 추가
}

void USkinnedMeshComponent::DuplicateSubObjects()
{
   Super::DuplicateSubObjects();

   // CPU 버퍼: 각 컴포넌트마다 새로 생성
   SkeletalMesh->CreateVertexBuffer(&VertexBuffer);

   // GPU 리소스: 각 컴포넌트마다 독립적으로 생성해야 함!
   // 복제된 컴포넌트가 원본의 작은 Constant Buffer를 공유하면 안 됨!
   GPUSkinnedVertexBuffer = nullptr;
   BoneMatrixConstantBuffer = nullptr;

   // GPU Query 리소스도 각 컴포넌트마다 독립적으로 생성
   GPUDisjointQuery = nullptr;
   GPUTimestampStartQuery = nullptr;
   GPUTimestampEndQuery = nullptr;
   GPUQueryFrameDelay = 0;


   // GPU 스키닝 모드면 새로운 256 크기 버퍼 생성
   if (bUseGPUSkinning)
   {
      CreateGPUSkinningResources();
   }

}

void USkinnedMeshComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData()) { return; }

   // IndexBuffer 유효성 검사 (PIE 중지 시 이미 해제되었을 수 있음)
   if (!SkeletalMesh->GetIndexBuffer())
   {
      return;
   }

   // CPU 스키닝 모드: 버텍스 버퍼 업데이트 (성능 측정 포함)
   if (!bUseGPUSkinning)
   {
      if (!VertexBuffer)
      {
         return;
      }

      if (bSkinningMatricesDirty)
      {
         auto BufferUploadStart = std::chrono::high_resolution_clock::now();
         //std::chrono::duration<double, std::milli> CalcDuration = BufferUploadStart - CPUSkinningStartTime;

         bSkinningMatricesDirty = false;

         // 버퍼 업로드
         SkeletalMesh->UpdateVertexBuffer(SkinnedVertices, VertexBuffer);
         auto CpuTimeEnd = std::chrono::high_resolution_clock::now();

         // CPU 스키닝 CPU 작업 시간 측정: PerformSkinning 시작 ~ 버퍼 업로드 끝
         //std::chrono::duration<double, std::milli> UploadDuration = CpuTimeEnd - BufferUploadStart;
         std::chrono::duration<double, std::milli> TotalCpuDuration = CpuTimeEnd - CPUSkinningStartTime;

         LastCPUSkinningCpuTime = TotalCpuDuration.count();

         // CPU 작업 시간 즉시 기록 (DrawIndexed GPU 시간은 ReadGPUQueryResults에서 추가)
         FSkinningStats::GetInstance().RecordCPUSkinningTime(LastCPUSkinningCpuTime);
      }
   }
   // GPU 스키닝 모드: 본 매트릭스 버퍼 업데이트
   else
   {
      if (!BoneMatrixConstantBuffer || !GPUSkinnedVertexBuffer)
      {
         return;
      }

      // GPU 스키닝: 상수 버퍼 업데이트 시간 측정 (CPU 작업)
      auto GpuCpuTimeStart = std::chrono::high_resolution_clock::now();
      UpdateBoneMatrixBuffer();
      auto GpuCpuTimeEnd = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> GpuCpuDuration = GpuCpuTimeEnd - GpuCpuTimeStart;

      LastGPUSkinningCpuTime = GpuCpuDuration.count();

      // GPU 작업 CPU 부분 즉시 기록 (DrawIndexed GPU 시간은 ReadGPUQueryResults에서 추가)
      FSkinningStats::GetInstance().RecordGPUSkinningTime(LastGPUSkinningCpuTime);
   }

    const TArray<FGroupInfo>& MeshGroupInfos = SkeletalMesh->GetMeshGroupInfo();
    auto DetermineMaterialAndShader = [&](uint32 SectionIndex) -> TPair<UMaterialInterface*, UShader*>
    {
       UMaterialInterface* Material = GetMaterial(SectionIndex);
       UShader* Shader = nullptr;

       if (Material && Material->GetShader())
       {
          Shader = Material->GetShader();
       }
       else
       {
          // UE_LOG("USkinnedMeshComponent: 머티리얼이 없거나 셰이더가 없어서 기본 머티리얼 사용 section %u.", SectionIndex);
          Material = UResourceManager::GetInstance().GetDefaultMaterial();
          if (Material)
          {
             Shader = Material->GetShader();
          }
          if (!Material || !Shader)
          {
             return { nullptr, nullptr };
          }
       }
       return { Material, Shader };
    };

    const bool bHasSections = !MeshGroupInfos.IsEmpty();
    const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;

    for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
    {
       uint32 IndexCount = 0;
       uint32 StartIndex = 0;

       if (bHasSections)
       {
          const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
          IndexCount = Group.IndexCount;
          StartIndex = Group.StartIndex;
       }
       else
       {
          IndexCount = SkeletalMesh->GetIndexCount();
          StartIndex = 0;
       }

       if (IndexCount == 0)
       {
          continue;
       }

       auto [MaterialToUse, ShaderToUse] = DetermineMaterialAndShader(SectionIndex);
       if (!MaterialToUse || !ShaderToUse)
       {
          continue;
       }

       FMeshBatchElement BatchElement;
       TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
       if (0 < MaterialToUse->GetShaderMacros().Num())
       {
          ShaderMacros.Append(MaterialToUse->GetShaderMacros());
       }

       // GPU 스키닝 모드일 때 GPU_SKINNING 매크로 추가
       if (bUseGPUSkinning)
       {
          FShaderMacro GPUSkinningMacro;
          GPUSkinningMacro.Name = "GPU_SKINNING";
          GPUSkinningMacro.Definition = "1";
          ShaderMacros.Add(GPUSkinningMacro);
       }

       FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);

       if (ShaderVariant)
       {
          BatchElement.VertexShader = ShaderVariant->VertexShader;
          BatchElement.PixelShader = ShaderVariant->PixelShader;
          BatchElement.InputLayout = ShaderVariant->InputLayout;
       }

       BatchElement.Material = MaterialToUse;

       // CPU/GPU 모드에 따라 다른 버텍스 버퍼와 스트라이드 사용
       if (bUseGPUSkinning)
       {
          BatchElement.VertexBuffer = GPUSkinnedVertexBuffer;
          BatchElement.VertexStride = sizeof(FSkinnedVertex);
          BatchElement.BoneMatrixConstantBuffer = BoneMatrixConstantBuffer;
          BatchElement.OwnerComponent = this;  // GPU Query를 위한 컴포넌트 설정
       }
       else
       {
          // CPU 스키닝: CPU에서 계산된 정점 사용
          BatchElement.VertexBuffer = VertexBuffer;
          BatchElement.VertexStride = SkeletalMesh->GetVertexStride();
          BatchElement.BoneMatrixConstantBuffer = nullptr;
          BatchElement.OwnerComponent = this;  // CPU 모드에서도 DrawIndexed GPU 시간 측정을 위해 설정
       }

       BatchElement.IndexBuffer = SkeletalMesh->GetIndexBuffer();

       BatchElement.IndexCount = IndexCount;
       BatchElement.StartIndex = StartIndex;
       BatchElement.BaseVertexIndex = 0;
       BatchElement.WorldMatrix = GetWorldMatrix();
       BatchElement.ObjectID = InternalIndex;
       BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

       OutMeshBatchElements.Add(BatchElement);
    }
}

FAABB USkinnedMeshComponent::GetWorldAABB() const
{
   return {};
   // const FTransform WorldTransform = GetWorldTransform();
   // const FMatrix WorldMatrix = GetWorldMatrix();
   //
   // if (!SkeletalMesh)
   // {
   //    const FVector Origin = WorldTransform.TransformPosition(FVector());
   //    return FAABB(Origin, Origin);
   // }
   //
   // const FAABB LocalBound = SkeletalMesh->GetLocalBound(); // <-- 이 함수 구현 필요
   // const FVector LocalMin = LocalBound.Min;
   // const FVector LocalMax = LocalBound.Max;
   //
   // // ... (이하 AABB 계산 로직은 UStaticMeshComponent와 동일) ...
   // const FVector LocalCorners[8] = {
   //    FVector(LocalMin.X, LocalMin.Y, LocalMin.Z),
   //    FVector(LocalMax.X, LocalMin.Y, LocalMin.Z),
   //    // ... (나머지 6개 코너) ...
   //    FVector(LocalMax.X, LocalMax.Y, LocalMax.Z)
   // };
   //
   // FVector4 WorldMin4 = FVector4(LocalCorners[0].X, LocalCorners[0].Y, LocalCorners[0].Z, 1.0f) * WorldMatrix;
   // FVector4 WorldMax4 = WorldMin4;
   //
   // for (int32 CornerIndex = 1; CornerIndex < 8; ++CornerIndex)
   // {
   //    const FVector4 WorldPos = FVector4(LocalCorners[CornerIndex].X
   //       , LocalCorners[CornerIndex].Y
   //       , LocalCorners[CornerIndex].Z
   //       , 1.0f)
   //       * WorldMatrix;
   //    WorldMin4 = WorldMin4.ComponentMin(WorldPos);
   //    WorldMax4 = WorldMax4.ComponentMax(WorldPos);
   // }
   //
   // FVector WorldMin = FVector(WorldMin4.X, WorldMin4.Y, WorldMin4.Z);
   // FVector WorldMax = FVector(WorldMax4.X, WorldMax4.Y, WorldMax4.Z);
   // return FAABB(WorldMin, WorldMax);
}

void USkinnedMeshComponent::OnTransformUpdated()
{
   Super::OnTransformUpdated();
   MarkWorldPartitionDirty();
}

void USkinnedMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
   ClearDynamicMaterials();

   SkeletalMesh = UResourceManager::GetInstance().Load<USkeletalMesh>(PathFileName);

   if (VertexBuffer)
   {
      VertexBuffer->Release();
      VertexBuffer = nullptr;
   }

   if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
   {
      SkeletalMesh->CreateVertexBuffer(&VertexBuffer);

      // GPU 스키닝 모드면 GPU 리소스도 생성
      if (bUseGPUSkinning)
      {
         CreateGPUSkinningResources();
      }

      const TArray<FMatrix> IdentityMatrices(SkeletalMesh->GetBoneCount(), FMatrix::Identity());
      UpdateSkinningMatrices(IdentityMatrices, IdentityMatrices);
      PerformSkinning();

      const TArray<FGroupInfo>& GroupInfos = SkeletalMesh->GetMeshGroupInfo();
       MaterialSlots.resize(GroupInfos.size());
       for (int i = 0; i < GroupInfos.size(); ++i)
      {
         // FGroupInfo에 InitialMaterialName이 있다고 가정
         SetMaterialByName(i, GroupInfos[i].InitialMaterialName);
      }
      MarkWorldPartitionDirty();
   }
   else
   {
      SkeletalMesh = nullptr;
      UpdateSkinningMatrices(TArray<FMatrix>(), TArray<FMatrix>());
      PerformSkinning();
   }
}

void USkinnedMeshComponent::PerformSkinning()
{
   if (bUseGPUSkinning) { return; }

   if (!SkeletalMesh || FinalSkinningMatrices.IsEmpty()) { return; }
   if (!bSkinningMatricesDirty) { return; }

   // CPU 스키닝 성능 측정 시작
   CPUSkinningStartTime = std::chrono::high_resolution_clock::now();

   const TArray<FSkinnedVertex>& SrcVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
   const int32 NumVertices = SrcVertices.Num();
   SkinnedVertices.SetNum(NumVertices);

   for (int32 Idx = 0; Idx < NumVertices; ++Idx)
   {
      const FSkinnedVertex& SrcVert = SrcVertices[Idx];
      FNormalVertex& DstVert = SkinnedVertices[Idx];

      DstVert.pos = SkinVertexPosition(SrcVert);
      DstVert.normal = SkinVertexNormal(SrcVert);
      DstVert.Tangent = SkinVertexTangent(SrcVert);
      DstVert.tex = SrcVert.UV;
   }
}

void USkinnedMeshComponent::UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices)
{
   FinalSkinningMatrices = InSkinningMatrices;
   FinalSkinningNormalMatrices = InSkinningNormalMatrices;
   bSkinningMatricesDirty = true;
}

FVector USkinnedMeshComponent::SkinVertexPosition(const FSkinnedVertex& InVertex) const
{
   FVector BlendedPosition(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
         FVector TransformedPosition = SkinMatrix.TransformPosition(InVertex.Position);
         BlendedPosition += TransformedPosition * Weight;
      }
   }

   return BlendedPosition;
}

FVector USkinnedMeshComponent::SkinVertexNormal(const FSkinnedVertex& InVertex) const
{
   FVector BlendedNormal(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningNormalMatrices[BoneIndex];
         FVector TransformedNormal = SkinMatrix.TransformVector(InVertex.Normal);
         BlendedNormal += TransformedNormal * Weight;
      }
   }

   return BlendedNormal.GetSafeNormal();
}

FVector4 USkinnedMeshComponent::SkinVertexTangent(const FSkinnedVertex& InVertex) const
{
   const FVector OriginalTangentDir(InVertex.Tangent.X, InVertex.Tangent.Y, InVertex.Tangent.Z);
   const float OriginalSignW = InVertex.Tangent.W;

   FVector BlendedTangentDir(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
         FVector TransformedTangentDir = SkinMatrix.TransformVector(OriginalTangentDir);
         BlendedTangentDir += TransformedTangentDir * Weight;
      }
   }

   const FVector FinalTangentDir = BlendedTangentDir.GetSafeNormal();
   return { FinalTangentDir.X, FinalTangentDir.Y, FinalTangentDir.Z, OriginalSignW };
}

// ===== GPU 스키닝 함수 구현 =====

void USkinnedMeshComponent::SetSkinningMode(bool bUseGPU)
{
   if (bUseGPUSkinning == bUseGPU)
   {
      return;
   }

   bUseGPUSkinning = bUseGPU;

   if (bUseGPUSkinning)
   {
      // GPU 스키닝: Vertex Buffer + Constant Buffer + GPU Query 생성
      if (!BoneMatrixConstantBuffer || !GPUSkinnedVertexBuffer)
      {
         CreateGPUSkinningResources();
      }
   }
   else
   {
      // CPU 스키닝: CPU에서 정점 계산, GPU Query는 DrawIndexed 측정용으로 유지
      bSkinningMatricesDirty = true;
      PerformSkinning();

      // CPU 스키닝에서도 DrawIndexed GPU 시간 측정을 위해 Query 리소스 생성
      if (!GPUDisjointQuery || !GPUTimestampStartQuery || !GPUTimestampEndQuery)
      {
         CreateGPUQueryResources();
      }
   }
}

void USkinnedMeshComponent::CreateGPUSkinningResources()
{
   if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
   {
      return;
   }

   ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();
   if (!Device)
   {
      return;
   }

   // 기존 리소스 해제
   ReleaseGPUSkinningResources();

   // 1. GPU용 Vertex Buffer 생성 (FSkinnedVertex 형식, 본 인덱스/가중치 포함)
   const TArray<FSkinnedVertex>& SkinnedVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
   const uint32 NumVertices = SkinnedVertices.Num();
   const uint32 VertexBufferSize = NumVertices * sizeof(FSkinnedVertex);

   D3D11_BUFFER_DESC VertexBufferDesc = {};
   VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;  // GPU 스키닝은 정점 데이터를 변경하지 않음
   VertexBufferDesc.ByteWidth = VertexBufferSize;
   VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
   VertexBufferDesc.CPUAccessFlags = 0;

   D3D11_SUBRESOURCE_DATA VertexInitData = {};
   VertexInitData.pSysMem = SkinnedVertices.data();

   HRESULT hr = Device->CreateBuffer(&VertexBufferDesc, &VertexInitData, &GPUSkinnedVertexBuffer);
   if (FAILED(hr))
   {
      return;
   }

   // 2. Bone Matrix Constant Buffer 생성 (register b6)
   // 셰이더와 일치: 항상 256개 본 크기로 고정 (16384 bytes)
   const uint32 MaxBones = 256;  // UberLit.hlsl의 BoneMatrices[256]과 일치
   const uint32 BoneBufferSize = MaxBones * sizeof(FMatrix);  // 256 * 64 = 16384 bytes


   D3D11_BUFFER_DESC ConstantBufferDesc = {};
   ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
   ConstantBufferDesc.ByteWidth = (BoneBufferSize + 15) & ~15;  // 16바이트 정렬

   ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
   ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

   hr = Device->CreateBuffer(&ConstantBufferDesc, nullptr, &BoneMatrixConstantBuffer);

   if (FAILED(hr))
   {
      if (GPUSkinnedVertexBuffer)
      {
         GPUSkinnedVertexBuffer->Release();
         GPUSkinnedVertexBuffer = nullptr;
      }
      return;
   }

   // 3. GPU Query 리소스 생성 (성능 측정용)
   CreateGPUQueryResources();
}

void USkinnedMeshComponent::ReleaseGPUSkinningResources()
{
   // GPU 작업이 완료될 때까지 대기 (중요: Release 전에 GPU가 리소스 사용 완료해야 함)
   //ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   //if (Context)
   //{
   //   // 모든 GPU 명령 완료 대기
   //   Context->Flush();
   //   Context->ClearState();
   //}
   // GPU Query 리소스 해제
   ReleaseGPUQueryResources();

   if (GPUSkinnedVertexBuffer)
   {
      GPUSkinnedVertexBuffer->Release();
      GPUSkinnedVertexBuffer = nullptr;
   }

   if (BoneMatrixConstantBuffer)
   {
      BoneMatrixConstantBuffer->Release();
      BoneMatrixConstantBuffer = nullptr;
   }
}

void USkinnedMeshComponent::UpdateBoneMatrixBuffer()
{
   // GPU 스키닝이 비활성화되었으면 리턴
   if (!bUseGPUSkinning)
   {
      return;
   }

   // 버퍼나 매트릭스 데이터가 없으면 리턴
   if (!BoneMatrixConstantBuffer || FinalSkinningMatrices.IsEmpty())
   {
      return;
   }

   ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   if (!Context)
   {
      return;
   }

   // SEH(Structured Exception Handling)로 예외 처리
   __try
   {
      // Constant Buffer에 본 매트릭스 업로드
      D3D11_MAPPED_SUBRESOURCE MappedResource;
      HRESULT hr = Context->Map(BoneMatrixConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

      if (FAILED(hr))
      {
         return;
      }

      const uint32 NumBones = FinalSkinningMatrices.Num();

      // 버퍼에 복사할 본 개수 결정 (버퍼 크기를 초과하지 않도록)
      // 버퍼는 실제 메시의 본 개수만큼 생성되어 있음
      const uint32 MaxBonesInBuffer = SkeletalMesh->GetBoneCount();
      const uint32 BonesToCopy = (NumBones < MaxBonesInBuffer) ? NumBones : MaxBonesInBuffer;

      // 본 매트릭스 복사
      memcpy(MappedResource.pData, FinalSkinningMatrices.data(), BonesToCopy * sizeof(FMatrix));

      Context->Unmap(BoneMatrixConstantBuffer, 0);
   }
   __except (EXCEPTION_EXECUTE_HANDLER)
   {
   }
}

// ===== GPU Query 관련 함수 =====

void USkinnedMeshComponent::CreateGPUQueryResources()
{

   ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();
   if (!Device)
   {
      return;
   }

   // Disjoint Query 생성 (GPU 주파수 변경 감지)
   D3D11_QUERY_DESC disjointDesc = {};
   disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
   HRESULT hr = Device->CreateQuery(&disjointDesc, &GPUDisjointQuery);
   if (FAILED(hr))
   {
      return;
   }

   // Timestamp Query 생성 - 시작
   D3D11_QUERY_DESC timestampDesc = {};
   timestampDesc.Query = D3D11_QUERY_TIMESTAMP;
   hr = Device->CreateQuery(&timestampDesc, &GPUTimestampStartQuery);
   if (FAILED(hr))
   {
      if (GPUDisjointQuery) { GPUDisjointQuery->Release(); GPUDisjointQuery = nullptr; }
      return;
   }

   // Timestamp Query 생성 - 종료
   hr = Device->CreateQuery(&timestampDesc, &GPUTimestampEndQuery);
   if (FAILED(hr))
   {
      if (GPUDisjointQuery) { GPUDisjointQuery->Release(); GPUDisjointQuery = nullptr; }
      if (GPUTimestampStartQuery) { GPUTimestampStartQuery->Release(); GPUTimestampStartQuery = nullptr; }
      return;
   }
}

void USkinnedMeshComponent::ReleaseGPUQueryResources()
{
   if (GPUDisjointQuery)
   {
      GPUDisjointQuery->Release();
      GPUDisjointQuery = nullptr;
   }

   if (GPUTimestampStartQuery)
   {
      GPUTimestampStartQuery->Release();
      GPUTimestampStartQuery = nullptr;
   }

   if (GPUTimestampEndQuery)
   {
      GPUTimestampEndQuery->Release();
      GPUTimestampEndQuery = nullptr;
   }
}

void USkinnedMeshComponent::BeginGPUQuery()
{
   //UE_LOG("[DEBUG] BeginGPUQuery 호출 (모드: %s, Delay: %d)", bUseGPUSkinning ? "GPU" : "CPU", GPUQueryFrameDelay);

   // GPU Query 중복 방지: 이미 대기 중인 Query가 있으면 스킵
   if (GPUQueryFrameDelay > 0)
   {
      //UE_LOG("[DEBUG] BeginGPUQuery: 이미 대기 중 (Delay: %d), 스킵", GPUQueryFrameDelay);
      return;
   }

   //UE_LOG("[DEBUG] BeginGPUQuery 시작 (모드: %s)", bUseGPUSkinning ? "GPU" : "CPU");

   if (!GPUDisjointQuery || !GPUTimestampStartQuery || !GPUTimestampEndQuery)
   {
      CreateGPUQueryResources();
      if (!GPUDisjointQuery || !GPUTimestampStartQuery || !GPUTimestampEndQuery)
      {
         return;
      }
   }

   ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   if (!Context)
   {
      return;
   }

   // DrawIndexed의 GPU 실행 시간 측정 시작 (CPU/GPU 스키닝 모두 적용)
   //UE_LOG("[DEBUG] BeginGPUQuery: Context->Begin(GPUDisjointQuery) 호출");
   Context->Begin(GPUDisjointQuery);
   //UE_LOG("[DEBUG] BeginGPUQuery: Context->End(GPUTimestampStartQuery) 호출");
   Context->End(GPUTimestampStartQuery);
   //UE_LOG("[DEBUG] BeginGPUQuery: Query Begin 완료");
}

void USkinnedMeshComponent::EndGPUQuery()
{
  // UE_LOG("[DEBUG] EndGPUQuery 호출 (모드: %s, Delay: %d)", bUseGPUSkinning ? "GPU" : "CPU", GPUQueryFrameDelay);

   // GPU Query 리소스 확인 및 중복 방지
   if (!GPUDisjointQuery || !GPUTimestampEndQuery || GPUQueryFrameDelay > 0)
   {
      if (GPUQueryFrameDelay > 0)
      {
         //UE_LOG("[DEBUG] EndGPUQuery: 이미 종료됨 (Delay: %d), 스킵", GPUQueryFrameDelay);
      }
      else
      {
      }
      return;
   }

   ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   if (!Context)
   {
      return;
   }


   // DrawIndexed의 GPU 실행 시간 측정 종료 (CPU/GPU 스키닝 모두 적용)
   Context->End(GPUTimestampEndQuery);
   Context->End(GPUDisjointQuery);
   GPUQueryFrameDelay = 5;  // 5프레임 후 결과 읽기 (GPU 작업 완료 대기)

}

void USkinnedMeshComponent::ReadGPUQueryResults()
{
   // GPU Query 결과 대기 중인지 확인
   if (GPUQueryFrameDelay <= 0)
   {
      return;
   }

   GPUQueryFrameDelay--;

   if (GPUQueryFrameDelay > 0)
   {
      //UE_LOG("[DEBUG] ReadGPUQueryResults: 대기 중... (남은 프레임: %d)", GPUQueryFrameDelay);
      return;
   }


   // Query 객체 유효성 검사
   if (!GPUDisjointQuery || !GPUTimestampStartQuery || !GPUTimestampEndQuery)
   {
      //UE_LOG("[ERROR] ReadGPUQueryResults: Query 객체가 nullptr! (Disjoint: %p, Start: %p, End: %p)",
             //GPUDisjointQuery, GPUTimestampStartQuery, GPUTimestampEndQuery);
      return;
   }

   ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   if (!Context)
   {
      //UE_LOG("[ERROR] ReadGPUQueryResults: Context is nullptr!");
      return;
   }

   // Disjoint Query 결과 읽기
   D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
   memset(&disjointData, 0, sizeof(disjointData));

   HRESULT hr = Context->GetData(GPUDisjointQuery, &disjointData, sizeof(disjointData), 0);


   if (hr != S_OK)
   {
      if (hr == S_FALSE)
      {
      }
      else
      {
      }
      GPUQueryFrameDelay = 1;
      return;
   }


   if (disjointData.Disjoint)
   {
      return;
   } 

   // Timestamp 시작/종료 읽기
   UINT64 startTime = 0, endTime = 0;
   hr = Context->GetData(GPUTimestampStartQuery, &startTime, sizeof(UINT64), 0);

   if (hr != S_OK)
   {
      GPUQueryFrameDelay = 1;
      return;
   }

   hr = Context->GetData(GPUTimestampEndQuery, &endTime, sizeof(UINT64), 0);

   if (hr != S_OK)
   {
      GPUQueryFrameDelay = 1;
      return;
   }

   // DrawIndexed GPU 실행 시간 계산 (밀리초)
   UINT64 delta = endTime - startTime;
   double gpuDrawTimeMS = (delta * 1000.0) / static_cast<double>(disjointData.Frequency);

   // CPU/GPU 모드별로 다른 통계 기록 및 로그 출력
   if (bUseGPUSkinning)
   {
      // GPU 스키닝: CPU 작업(상수 버퍼 업데이트) + GPU 작업(스키닝 계산 + 그리기)
      double totalTimeMS = LastGPUSkinningCpuTime + gpuDrawTimeMS;

   /*   UE_LOG("[DEBUG] [GPU 스키닝] DrawIndexed(GPU): %.6f ms, 상수버퍼(CPU): %.6f ms, 총합: %.6f ms",
             gpuDrawTimeMS, LastGPUSkinningCpuTime, totalTimeMS);
       */
      FSkinningStats::GetInstance().RecordGPUSkinningTime(totalTimeMS);
   }
   else
   {
      // CPU 스키닝: CPU 작업(정점 계산 + 버퍼 업로드) + GPU 작업(그리기만)
      double totalTimeMS = LastCPUSkinningCpuTime + gpuDrawTimeMS;

    /*  UE_LOG("[DEBUG] [CPU 스키닝] DrawIndexed(GPU): %.6f ms, 정점계산+업로드(CPU): %.6f ms, 총합: %.6f ms",
             gpuDrawTimeMS, LastCPUSkinningCpuTime, totalTimeMS);*/

      FSkinningStats::GetInstance().RecordCPUSkinningTime(totalTimeMS);
   }
}
