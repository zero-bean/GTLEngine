#pragma once

/**
 * @brief 언리얼 엔진 'CoreMiscDefines.h'에 정의되어있는 값이다.
 * @note 가급적이면 TArray::INDEX_NONE과 일치시켜야 한다.
 */
enum {INDEX_NONE = -1};

/**
 * @brief 레퍼런스-스켈레톤 관련 정보를 보유하는 구조체이다.
 * @note 본 트랜스폼은 FTransform 배열로 저장된다.
 * @todo 직렬화를 어떻게 할 것인지에 대해서 고려해야한다.
 */
struct FMeshBoneInfo
{
	// 본의 이름
	FName Name;

	// 루트 본일 경우 INDEX_NONE(= -1)
	int32 ParentIndex;

	FMeshBoneInfo() : Name(FName::GetNone()), ParentIndex(INDEX_NONE) {}

	bool operator==(const FMeshBoneInfo& Other) const
	{
		return Name == Other.Name;
	}
};

class FReferenceSkeleton
{
public:
	/** @note 현재 단계에서는 Control Rig같은 멀티-루트 스켈레탈 메시는 사용되지 않는다고 가정한다. */
	FReferenceSkeleton(/** bool bInOnlyOneRootAllowed = true */)
		/** : bOnlyOneRootAllowed(bInOnlyOneRootAllowed) */
	{}

private:
	/** 직렬화할 레퍼런스 본 정보 */
	TArray<FMeshBoneInfo> RawRefBoneInfo;

	/**
	 * 레퍼런스 본 트랜스폼
	 * @todo 부모 본을 기준으로 하는 로컬 트랜스폼인지, 모델 트랜스폼인지 확인해야 한다 (전자일 것으로 추정).
	 */
	TArray<FTransform> RawRefBonePose;

	/** 언리얼 엔진에서는 Final 본 정보를 저장하지만, 구현의 편의를 위해서 Raw 정보만 사용한다고 가정한다. */

	/** 본 인덱스를 본 이름으로부터 탐색하는 TMap */
	TMap<FName, int32> RawNameToIndexMap;

	/** 지정된 본의 자식이 없을 경우에만 제거한다. 제거 성공 여부를 반환한다. */
	bool RemoveIndividualBone(int32 BoneIndex, TArray<int32>& OutBonesRemoved);

	int32 GetParentIndexInternal(const int32 BoneIndex, const TArray<FMeshBoneInfo>& BoneInfo) const;

	void UpdateRefPoseTransform(const int32 BoneIndex, const FTransform& BonePose)
	{
		RawRefBonePose[BoneIndex] = BonePose;
	}

	/**
	 * 새로운 본을 삽입한다.
	 * BoneName은 이미 존재해서는 안된다. 또한, 부모 인덱스는 유효해야한다.
	 */
	void Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose);

	/** InBoneName을 제거하고, bRemoveChildren = true일 경우 그 자식까지 제거한다. */
	void Remove(const FName InBoneName, const bool bRemoveChildren);

	/** InBoneName을 InNewName로 이름을 변경한다. */
	void Rename(const FName InBoneName, const FName InNewName);

	/** InParentName을 InBoneName의 부모로 설정한다. */
	void SetParent(const FName InBoneName, const FName InParentName);

public:
	/**
	 * @brief 외부 데이터로부터 스켈레톤을 초기화한다.
	 */
	void InitializeFromData(const TArray<FMeshBoneInfo>& BoneInfos, const TArray<FTransform>& BonePoses)
	{
		RawRefBoneInfo = BoneInfos;
		RawRefBonePose = BonePoses;

		RawNameToIndexMap.Empty();
		for (int32 i = 0; i < RawRefBoneInfo.Num(); ++i)
		{
			RawNameToIndexMap.Add(RawRefBoneInfo[i].Name, i);
		}
	}

	/** 스켈레톤의 raw 본 개수를 반환한다. 에셋의 원래 본 개수를 나타낸다. */
	int32 GetRawBoneNum() const
	{
		return RawRefBoneInfo.Num();
	}

	/** 내부 private 데이터에 대한 접근자이다. */
	const TArray<FMeshBoneInfo>& GetRawRefBoneInfo() const
	{
		return RawRefBoneInfo;
	}

	/** 내부 private 데이터에 대한 접근자이다. */
	const TArray<FTransform>& GetRawRefBonePose() const
	{
		return RawRefBonePose;
	}

	const TMap<FName, int32>& GetRawNameToIndexMap() const { return RawNameToIndexMap;}

	/** Raw 본 이름에 대한 배열을 반환한다. */
	TArray<FName> GetRawRefBoneNames() const
	{
		TArray<FName> BoneNames;
		BoneNames.Reserve(RawRefBoneInfo.Num());

		for (const FMeshBoneInfo& BoneInfo : RawRefBoneInfo)
		{
			BoneNames.Add(BoneInfo.Name);
		}

		return BoneNames;
	}

	/** BoneName으로부터 본 인덱스를 찾는다. */
	int32 FindRawBoneIndex(const FName& BoneName) const
	{
		int32 BoneIndex = INDEX_NONE;
		if (BoneName != FName::GetNone())
		{
			const int32* IndexPtr = RawNameToIndexMap.Find(BoneName);
			if (IndexPtr)
			{
				BoneIndex = *IndexPtr;
			}
		}
		return BoneIndex;
	}

	/** 언리얼 엔진에는 없는 함수이지만, 편의를 위해 추가한다. */
	FName GetRawBoneName(const int32 BoneIndex) const
	{
		return RawRefBoneInfo[BoneIndex].Name;
	}

	int32 GetRawParentIndex(const int32 BoneIndex) const
	{
		return GetParentIndexInternal(BoneIndex, RawRefBoneInfo);
	}

	bool IsValidRawIndex(int32 Index) const
	{
		return (RawRefBoneInfo.IsValidIndex(Index));
	}
};
