#include "pch.h"

#include "Runtime/Engine/Public/ReferenceSkeleton.h"

int32 FReferenceSkeleton::GetParentIndexInternal(const int32 BoneIndex, const TArray<FMeshBoneInfo>& BoneInfo) const
{
	const int32 ParentIndex = BoneInfo[BoneIndex].ParentIndex;

	return ParentIndex;
}
