#include "pch.h"

#include "Runtime/Engine/Public/SkeletalMesh.h"

IMPLEMENT_CLASS(USkeletalMesh, USkinnedAsset)

FSkeletalMeshRenderData* USkeletalMesh::GetSkeletalMeshRenderData() const
{
	return SkeletalMeshRenderData.Get();
}

void USkeletalMesh::SetSkeletalMeshRenderData(FSkeletalMeshRenderData* InRenderData)
{
	SkeletalMeshRenderData.Reset(InRenderData);
}

const TArray<FNormalVertex>& USkeletalMesh::GetVertices() const
{
	return StaticMesh->GetVertices();
}

const TArray<uint32>& USkeletalMesh::GetIndices() const
{
	return StaticMesh->GetIndices();
}

void USkeletalMesh::CalculateInvRefMatrices()
{
	const int32 NumRealBones = GetRefSkeleton().GetRawBoneNum();

	const TArray<FTransform>& RefBonePose = GetRefSkeleton().GetRawRefBonePose();

	TArray<FMatrix>& ComposedRefPoseMatrices_Matrix = GetCachedComposedRefPoseMatrices();

	TArray<FTransform> ComposedRefPoseTransforms;
	ComposedRefPoseTransforms.SetNum(NumRealBones);

	if (GetRefBasesInvMatrix().Num() != NumRealBones)
	{
		GetRefBasesInvMatrix().SetNum(NumRealBones);
		ComposedRefPoseMatrices_Matrix.SetNum(NumRealBones);

		for (int32 b = 0; b < NumRealBones; ++b)
		{
			// 1. 로컬 FTransform을 가져옵니다.
			const FTransform& LocalTransform = RefBonePose[b];

			if (b > 0)
			{
				const int32 Parent = GetRefSkeleton().GetRawParentIndex(b);

				ComposedRefPoseTransforms[b] = LocalTransform * ComposedRefPoseTransforms[Parent];
			}
			else
			{
				ComposedRefPoseTransforms[b] = LocalTransform;
			}

			FMatrix ComponentMatrix = ComposedRefPoseTransforms[b].ToMatrixWithScale();

			ComposedRefPoseMatrices_Matrix[b] = ComponentMatrix;
			GetRefBasesInvMatrix()[b] = ComponentMatrix.Inverse();
		}
	}
}

FMatrix USkeletalMesh::GetRefPoseMatrix(int32 BoneIndex) const
{
	assert(BoneIndex >= 0 && BoneIndex < GetRefSkeleton().GetRawBoneNum());
	FTransform BoneTransform = GetRefSkeleton().GetRawRefBonePose()[BoneIndex];
	return BoneTransform.ToMatrixWithScale();
}

FMatrix USkeletalMesh::GetComposedRefPoseMatrix(FName InBoneName)
{
	FMatrix LocalPose(FMatrix::Identity());

	if (InBoneName != FName::GetNone())
	{
		int32 BoneIndex = GetRefSkeleton().FindRawBoneIndex(InBoneName);
		if (BoneIndex != INDEX_NONE)
		{
			return GetComposedRefPoseMatrix(BoneIndex);
		}
	}
	return LocalPose;
}

FMatrix USkeletalMesh::GetComposedRefPoseMatrix(int32 InBoneIndex)
{
	return GetCachedComposedRefPoseMatrices()[InBoneIndex];
}
