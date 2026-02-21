#pragma once

#include "Global/CoreTypes.h"
#include "ReferenceSkeleton.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "SkinnedAsset.h"

/** 컨테이너 타입 정의 */
using FVertexArray = TArray<FVertex>;

/*-----------------------------------------------------------------------------
	FSkeletalMeshSourceModel
 -----------------------------------------------------------------------------*/

/**
 * @brief 에셋을 임포트할 때 생성되는 원본 데이터를 저장한다.
 * @note 언리얼 엔진에서는 'FMeshDescription'을 통해서 메시 정보를 관리하지만, 복잡성을 줄이기 위해서 FSkeletalMeshSourceModel이 직접 정보를 관리한다.
 * @todo 필요시 구현해서 사용한다. 런타임용 정보를 관리하는 FSkeletalMeshRenderData를 우선 구현한다.
 */
class FSkeletalMeshSourceModel
{
	/** @todo 필요시 구현해서 사용한다. */
};

/**
 * @brief 정점 하나에 대한 원본 스킨 가중치 정보
 */
struct FRawSkinWeight
{
	static constexpr uint32 MAX_TOTAL_INFLUENCES = 12;

	FBoneIndexType InfluenceBones[MAX_TOTAL_INFLUENCES];
	uint16 InfluenceWeights[MAX_TOTAL_INFLUENCES];
};

/**
 * @brief GPU에 전송될 런타임용 데이터를 저장한다.
 * @note 언리얼 엔진에서는 'FSkeletalMeshLODRenderData'의 배열을 통해서 LOD를 이용하지만, 복잡성을 줄이기 위해 직접 FSkeletalMeshRenderData가 정보를 관리한다.
 */
class FSkeletalMeshRenderData
{
public:
	/**
	 * @note 이 배열의 인덱스는 SkeletalVertices의 정점과 일대일로 대응되어야 한다.
	 *		 즉, SkinWeightVertices[i]는 SkeletalVertices의 i번째 정점에 대한 정보이다.
	 *		 언리얼 엔진에서는 이 정보를 압축해서 GPU로 전달하지만, 여기에서는 직접 전달한다.
	 */
	TArray<FRawSkinWeight> SkinWeightVertices;
};

/*-----------------------------------------------------------------------------
	USkeletalMesh
 -----------------------------------------------------------------------------*/

/**
 * @brief 스켈레탈 메시는 뼈대 계층 구조에 결합되어 변형 애니메이션을 위해 사용할 수 있는 기하학적 형태이다.
 * 스켈레탈 메시는 메시의 표면을 구성하는 폴리곤 집합과, 이 폴리곤을 애니메이션화하기 위해 사용되는 계층적 골격으로 구성되어 있다.
 * 3D 모델, 리깅, 그리고 애니메이션은 외부 모델링 및 애니메이션 애플리케이션(3DSMax, Maya, Softimage, etc)에서 제작된다.
 */
UCLASS()
class USkeletalMesh : public USkinnedAsset
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMesh, USkinnedAsset)

public:
	USkeletalMesh() = default;
	virtual ~USkeletalMesh() = default;

	/*-----------------------------------------------------------------------------
		FSkeletalMeshRenderData (런타임에 사용되는 렌더링 리소스)
	 -----------------------------------------------------------------------------*/
public:
	FSkeletalMeshRenderData* GetSkeletalMeshRenderData() const;

	/** 스켈레탈 메시의 렌더 데이터 설정 */
	void SetSkeletalMeshRenderData(FSkeletalMeshRenderData* InRenderData);

	/** 스켈레탈 메시의 정점 데이터 접근 (StaticMesh->GetVertices()를 반환) */
	const TArray<FNormalVertex>& GetVertices() const;

	/** 스켈레탈 메시의 인덱스 데이터 접근 (StaticMesh->GetIndices()를 반환) */
	const TArray<uint32>& GetIndices() const;

	/** 스켈레탈 메시의 StaticMesh 접근 */
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	/** 스켈레탈 메시의 StaticMesh 설정 */
	void SetStaticMesh(UStaticMesh* InStaticMesh) { StaticMesh = InStaticMesh; }

private:
	/** 런타임에 사용되는 렌더링 리소스 */
	TUniquePtr<FSkeletalMeshRenderData> SkeletalMeshRenderData;

	/** 지오메트리 데이터를 담고 있는 StaticMesh */
	UStaticMesh* StaticMesh = nullptr;
	/*-----------------------------------------------------------------------------
		FReferenceSkeleton (본 계층 구조)
	 -----------------------------------------------------------------------------*/
public:
	virtual FReferenceSkeleton& GetRefSkeleton() override
	{
		return RefSkeleton;
	}

	virtual const FReferenceSkeleton& GetRefSkeleton() const override
	{
		return RefSkeleton;
	}

private:
	/** 스켈레탈 메시의 본 계층 구조를 포함하는 데이터 */
	FReferenceSkeleton RefSkeleton;

	/*-----------------------------------------------------------------------------
		RefBasesInvMatrix (정점을 본 공간으로 변환하는 행렬)
	 -----------------------------------------------------------------------------*/
public:
	void CalculateInvRefMatrices();

	virtual TArray<FMatrix>& GetRefBasesInvMatrix() override
	{
		return RefBasesInvMatrix;
	}

	virtual const TArray<FMatrix>& GetRefBasesInvMatrix() const override
	{
		return RefBasesInvMatrix;
	}

	TArray<FMatrix>& GetCachedComposedRefPoseMatrices()
	{
		return CachedComposedRefPoseMatrices;
	}

	const TArray<FMatrix>& GetCachedComposedRefPoseMatrices() const
	{
		return CachedComposedRefPoseMatrices;
	}

	FMatrix GetRefPoseMatrix(int32 BoneIndex) const;

	FMatrix GetComposedRefPoseMatrix(FName InBoneName);
	FMatrix GetComposedRefPoseMatrix(int32 InBoneIndex);

	void SetRefBasesInvMatrix(const TArray<FMatrix>& InRefBasesInvMatrix)
	{
		RefBasesInvMatrix = InRefBasesInvMatrix;
	}

private:
	TArray<FMatrix> CachedComposedRefPoseMatrices;

	TArray<FMatrix> RefBasesInvMatrix;
};
