#pragma once
#include "MeshComponent.h"
#include "SkeletalMesh.h"
#include "USkinnedMeshComponent.generated.h"

UCLASS(DisplayName="스킨드 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkinnedMeshComponent : public UMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()

    USkinnedMeshComponent();
    ~USkinnedMeshComponent() override;

    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    void DuplicateSubObjects() override;
    
// Mesh Component Section
public:
    void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;
    
    FAABB GetWorldAABB() const override;
    void OnTransformUpdated() override;

// Skeletal Section
public:
    /**
     * @brief 렌더링할 스켈레탈 메시 에셋 설정 (UStaticMeshComponent::SetStaticMesh와 동일한 역할)
     * @param PathFileName 새 스켈레탈 메시 에셋 경로
     */
    virtual void SetSkeletalMesh(const FString& PathFileName);
    /**
     * @brief 이 컴포넌트의 USkeletalMesh 에셋을 반환
     */
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

    /**
     * @brief GPU/CPU 스키닝 모드 전환
     * @param bUseGPU true면 GPU 스키닝, false면 CPU 스키닝
     */
    void SetSkinningMode(bool bUseGPU);

    /**
     * @brief 현재 스키닝 모드 반환
     */
    bool IsUsingGPUSkinning() const { return bUseGPUSkinning; }

    /**
     * @brief GPU Query 시작 (Draw Call 전, SceneRenderer에서 사용)
     */
    void BeginGPUQuery();

    /**
     * @brief GPU Query 종료 (Draw Call 후, SceneRenderer에서 사용)
     */
    void EndGPUQuery();

protected:
    void PerformSkinning();
    /**
     * @brief 자식에게서 원본 메시를 받아 CPU 스키닝을 수행
     * @param InSkinningMatrices 스키닝 매트릭스
     */
    void UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices);

    /**
     * @brief GPU 스키닝용 본 매트릭스 상수 버퍼 업데이트
     */
    void UpdateBoneMatrixBuffer();

    /**
     * @brief GPU 스키닝용 리소스 생성 (Vertex Buffer + Constant Buffer)
     */
    void CreateGPUSkinningResources();

    /**
     * @brief GPU 스키닝용 리소스 해제
     */
    void ReleaseGPUSkinningResources();

    /**
     * @brief GPU Query 리소스 생성 (성능 측정용)
     */
    void CreateGPUQueryResources();

    /**
     * @brief GPU Query 리소스 해제
     */
    void ReleaseGPUQueryResources();

    /**
     * @brief GPU Query 결과 읽기 (TickComponent에서 호출)
     */
    void ReadGPUQueryResults();

    UPROPERTY(EditAnywhere, Category = "Skeletal Mesh", Tooltip = "Skeletal mesh asset to render")
    USkeletalMesh* SkeletalMesh;

    /**
     * @brief CPU 스키닝 최종 결과물. 렌더러가 이 데이터를 사용합니다.
     */
    TArray<FNormalVertex> SkinnedVertices;
    /**
     * @brief CPU 스키닝 최종 결과물. 렌더러가 이 데이터를 사용합니다.
     */
    TArray<FNormalVertex> NormalSkinnedVertices;

private:
    FVector SkinVertexPosition(const FSkinnedVertex& InVertex) const;
    FVector SkinVertexNormal(const FSkinnedVertex& InVertex) const;
    FVector4 SkinVertexTangent(const FSkinnedVertex& InVertex) const;

    /**
     * @brief 자식이 계산해 준, 현재 프레임의 최종 스키닝 행렬
    */
    TArray<FMatrix> FinalSkinningMatrices;
    TArray<FMatrix> FinalSkinningNormalMatrices;
    bool bSkinningMatricesDirty = true;

    /**
     * @brief CPU 스키닝에서 진행하기 때문에, Component별로 VertexBuffer를 가지고 스키닝 업데이트를 진행해야함
    */
    ID3D11Buffer* VertexBuffer = nullptr;

    // ===== GPU 스키닝 관련 멤버 =====
    /**
     * @brief GPU 스키닝 사용 여부
     */
    bool bUseGPUSkinning = true;

    /**
     * @brief GPU 스키닝용 정점 버퍼 (본 인덱스/가중치 포함, FSkinnedVertex 형식)
     */
    ID3D11Buffer* GPUSkinnedVertexBuffer = nullptr;

    /**
     * @brief GPU 스키닝용 본 매트릭스 상수 버퍼 (register b6)
     */
    ID3D11Buffer* BoneMatrixConstantBuffer = nullptr;

    /**
     * @brief GPU 리소스 소유권 플래그 (PIE 복제본은 false, 원본은 true)
     * PIE 복제본은 원본의 GPU 리소스를 공유하므로 소멸 시 해제하지 않음
     */
    //bool bOwnsGPUResources = true;

    // ===== GPU 성능 측정 관련 =====
    // CPU/GPU 스키닝 모두 DrawIndexed의 GPU 실행 시간을 측정하기 위해 사용
    /**
     * @brief GPU Timestamp Disjoint Query (GPU 주파수 변경 감지)
     */
    ID3D11Query* GPUDisjointQuery = nullptr;

    /**
     * @brief GPU Timestamp Query - 시작 시점 (DrawIndexed 전)
     */
    ID3D11Query* GPUTimestampStartQuery = nullptr;

    /**
     * @brief GPU Timestamp Query - 종료 시점 (DrawIndexed 후)
     */
    ID3D11Query* GPUTimestampEndQuery = nullptr;

    /**
     * @brief Query 결과 대기 프레임 카운터 (GPU는 비동기이므로 N프레임 후 읽기)
     */
    int32 GPUQueryFrameDelay = 0;

    /**
     * @brief CPU 스키닝의 CPU 작업 시간 (정점 계산 + 버퍼 업로드)
     */
    double LastCPUSkinningCpuTime = 0.0;

    /**
     * @brief GPU 스키닝의 CPU 작업 시간 (상수 버퍼 업데이트)
     */
    double LastGPUSkinningCpuTime = 0.0;

    /**
     * @brief CPU 스키닝 시작 시간 (정점 계산 시작)
     */
    std::chrono::high_resolution_clock::time_point CPUSkinningStartTime;
};
