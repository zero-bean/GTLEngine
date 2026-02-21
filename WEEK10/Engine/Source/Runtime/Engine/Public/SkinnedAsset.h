#pragma once

typedef uint16 FBoneIndexType;

UCLASS()
class USkinnedAsset : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(USkinnedAsset, UObject)

public:
	USkinnedAsset() = default;
	virtual ~USkinnedAsset() = default;

	/** 레퍼런스 스켈레톤을 반환한다. */
	virtual FReferenceSkeleton& GetRefSkeleton() = 0;
	virtual const FReferenceSkeleton& GetRefSkeleton() const = 0;

	/** 사전 계산된 레퍼런스 스켈레톤의 기저를 반환한다. */
	virtual TArray<FMatrix>& GetRefBasesInvMatrix() = 0;
	virtual const TArray<FMatrix>& GetRefBasesInvMatrix() const = 0;

	/* @todo 머티리얼 관련 함수
	virtual bool IsValidMaterialIndex(int32 Index) const;

	virtual TArray<FSkeletalMesh>& GetMeterials();
	virtual const TArray<FSkeletalMesh>& GetMeterials() const;

	virtual bool IsMaterialUsed(int32 MaterialIndex) const;
	*/

	/**
	 * @brief BoneSpaceTransforms 배열을 받아서 컴포넌트-공간 본 변환 행렬(ComponentSpaceTransforms)을 업데이트한다.
	 *
	 * 이 함수는 부모의 컴포넌트-공간 변환을 자식의 상대적인 변환을 계층적으로 곱함으로써 작동한다.
	 *
	 * @param InBoneSpaceTransforms 본의 로컬 공간 기준 변환 (부모 본에 상대적인 변환)
	 * @param InFillComponentSpaceTransformsRequiredBones 처리해야 할 본들의 인덱스 목록 (부모 본이 자식 본보다 먼저 오도록 정렬되어야 함, 언리얼 엔진에서는 LOD 등의 최적화를 위해 모든 본을 사용하지 않을 수도 있음)
	 * @param OutComponentSpaceTransforms 출력용 본의 컴포넌트 공간 기준 변환
	 */
	void FillComponentSpaceTransforms(const TArray<FTransform>& InBoneSpaceTransforms,
	                                  const TArray<FBoneIndexType>& InFillComponentSpaceTransformsRequiredBones,
	                                  TArray<FTransform>& OutComponentSpaceTransforms) const;
};
