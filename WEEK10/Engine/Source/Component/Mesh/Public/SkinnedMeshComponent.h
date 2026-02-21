#pragma once

#include "MeshComponent.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"
#include "Runtime/Engine/Public/SkinnedAsset.h"

class USkinnedAsset;

UENUM()
enum EBoneVisibilityStatus : int
{
	/** 부모가 숨겨져있을 경우, 본을 숨긴다. */
	BVS_HiddenByParent,
	/** 본을 시각화한다. */
	BVS_Visible,
	/** 본을 직접 숨긴다. */
	BVS_ExplicitlyHidden
};

/**
 * @brief 본 스킨드(bone skinned) 메쉬 렌더링을 지원하는 컴포넌트이다.
 * @note 이 클래스는 애니메이션을 지원하지 않는다.
 *
 * @see USkeletalMeshComponent
 */
UCLASS()
class USkinnedMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)

private:
	/** 이 클래스에 의해 사용되는 스킨드 에셋 */
	TObjectPtr<USkinnedAsset> SkinnedAsset;

public:
	USkinnedMeshComponent() = default;
	virtual ~USkinnedMeshComponent() = default;

	/*-----------------------------------------------------------------------------
		UObject 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	virtual UObject* Duplicate() override;

	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	/*-----------------------------------------------------------------------------
		UActorComponent 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime) override;

	virtual void EndPlay() override;

	/*-----------------------------------------------------------------------------
		UPrimitiveComponent 인터페이스
	 -----------------------------------------------------------------------------*/

	/** @todo 오버라이딩이 필요한 함수 확인 */

	/*-----------------------------------------------------------------------------
		USkinnedMeshComponent 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	/** @brief 이 메쉬를 위해 렌더링되는 SkinnedAsset을 반환한다. */
	USkinnedAsset* GetSkinnedAsset() const
	{
		return SkinnedAsset;
	}

	/** @brief 이 컴포넌트를 초기화하지 않고 SkinnedAsset을 바꾼다. */
	void SetSkinnedAsset(class USkinnedAsset* InSkinnedAsset)
	{
		SkinnedAsset = InSkinnedAsset;
	}

	/**  @brief 본 트랜스폼을 갱신한다. 각 클래스들은 이 함수를 구현해야한다. */
	virtual void RefreshBoneTransforms() = 0;

	/** @brief 틱 동안 수행해야하는 동작들을 수행한다(e.g., 애니메이션). RefreshBoneTransforms 이전에 호출되어야 한다. */
	virtual void TickPose(float DeltaTime);

	/** @brief 컴포넌트 공간 변환의 수를 반환한다. */
	int32 GetNumComponentSpaceTransforms() const
	{
		return GetComponentSpaceTransforms().Num();
	}

	/** @brief 읽기 전용으로 ComponentSpaceTransformArray를 반환한다. */
	const TArray<FTransform>& GetComponentSpaceTransforms() const
	{
		return ComponentSpaceTransformArray;
	}

	/** @brief 수정 가능한 형태로 ComponentSpaceTransformArray를 반환한다. */
	TArray<FTransform>& GetEditableComponentSpaceTransform()
	{
		return ComponentSpaceTransformArray;
	}

	/** @brief 스켈레톤의 본 수를 반환한다. */
	int32 GetNumBones() const
	{
		return GetSkinnedAsset() ? GetSkinnedAsset()->GetRefSkeleton().GetRawBoneNum() : 0;
	}

	/** @brief 지정된 본을 숨긴다. */
	virtual void HideBone(int32 BoneIndex);

	/** @brief 지정된 본 숨김을 해제한다. */
	virtual void UnHideBone(int32 BoneIndex);

	/** @brief 지정된 본이 숨겨져있는지 결정한다. */
	bool IsBoneHidden(int32 BoneIndex);

	/** @brief 읽기 전용으로 BoneVisibilityStates를 반환한다. */
	const TArray<uint8>& GetBoneVisibilityStates() const
	{
		return BoneVisibilityStates;
	}

	/** @brief 수정가능한 형태로 BoneVisibilityStates를 반환한다. */
	TArray<uint8>& GetEditableBoneVisibilityStates()
	{
		return BoneVisibilityStates;
	}

protected:
	/** 본의 시각화 여부를 결정하는 배열 (EBoneVisibilityStatus의 값을 각 본에 대해서 보유하고 있다. 정확히 값이 1(BVS_Visible)인 경우에만 보인다.)*/
	TArray<uint8> BoneVisibilityStates;

private:
	/**
	 * @brief 메쉬를 렌더링하기 위해 매 프레임 업데이트하는 컴포넌트-공간 본 행렬의 임시 배열.
	 * 
	 * 루트부터 해당 본까지의 모든 부모 본들의 Transform을 곱하여 계산된다.
	 * @note 언리얼 엔진에서는 더블 버퍼링을 수행하지만 여기서는 하나의 버퍼만 활용한다.
	 */
	TArray<FTransform> ComponentSpaceTransformArray;
};
