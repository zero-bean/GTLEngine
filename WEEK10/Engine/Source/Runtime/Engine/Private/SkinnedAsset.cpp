#include "pch.h"

#include "Runtime/Engine/Public/ReferenceSkeleton.h"
#include "Runtime/Engine/Public/SkinnedAsset.h"

#include <sol/stack_core.hpp>

IMPLEMENT_ABSTRACT_CLASS(USkinnedAsset, UObject)

void USkinnedAsset::FillComponentSpaceTransforms(const TArray<FTransform>& InBoneSpaceTransforms,
                                                 const TArray<FBoneIndexType>& InFillComponentSpaceTransformsRequiredBones,
                                                 TArray<FTransform>& OutComponentSpaceTransforms) const
{
	assert(GetRefSkeleton().GetRawBoneNum() == InBoneSpaceTransforms.Num());
	assert(GetRefSkeleton().GetRawBoneNum() == OutComponentSpaceTransforms.Num());

	const int32 NumBones = InBoneSpaceTransforms.Num();

	if (!NumBones)
	{
		return;
	}

	const FTransform* LocalTransformsData = InBoneSpaceTransforms.GetData();
	FTransform* ComponentSpaceData = OutComponentSpaceTransforms.GetData();

	// 첫번째 본은 항상 루트 본이어야 하며, 부모가 없어야 한다.
	assert(InFillComponentSpaceTransformsRequiredBones.Num() == 0 || InFillComponentSpaceTransformsRequiredBones[0] == 0);
	OutComponentSpaceTransforms[0] = InBoneSpaceTransforms[0];

	for (int32 i = 1; i < InFillComponentSpaceTransformsRequiredBones.Num(); ++i)
	{
		const int32 BoneIndex = InFillComponentSpaceTransformsRequiredBones[i];
		FTransform* SpaceBase = ComponentSpaceData + BoneIndex;

		const int32 ParentIndex = GetRefSkeleton().GetRawParentIndex(BoneIndex);
		FTransform* ParentSpaceBase = ComponentSpaceData + ParentIndex;

		*SpaceBase = *(LocalTransformsData + BoneIndex) * *ParentSpaceBase;
	}
}
