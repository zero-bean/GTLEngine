#include "pch.h"

#include <numeric>

#include "Component/Mesh/Public/SkeletalMeshComponent.h"

#include "Manager/Asset/Public/FbxManager.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "Utility/Public/JsonSerializer.h"
#include "Texture/Public/Texture.h"
#include "Core/Public/ObjectIterator.h"

IMPLEMENT_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

USkeletalMeshComponent::USkeletalMeshComponent()
	: bPoseDirty(false)
	, bSkinningDirty(false)
	, bNormalMapEnabled(false)
{
	FName DefaultFbxPath = "Data/DefaultSkeletalMesh.fbx";
	LoadSkeletalMeshAsset(DefaultFbxPath);
}

USkeletalMeshComponent::~USkeletalMeshComponent()
{
	if (VertexBuffer)
	{
		SafeRelease(VertexBuffer);
	}

	if (IndexBuffer)
	{
		SafeRelease(IndexBuffer);
	}
}

UObject* USkeletalMeshComponent::Duplicate()
{
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Super::Duplicate());

	//SkeletalMeshComponent->SkeletalMeshAsset = Cast<USkeletalMesh>(SkeletalMeshAsset->Duplicate()); // 만약, Viewer에서 Bone자체를 수정하는 기능을 넣는다면 Deep Copy 필요
	SkeletalMeshComponent->SkeletalMeshAsset = SkeletalMeshAsset;

	SkeletalMeshComponent->BoneSpaceTransforms = BoneSpaceTransforms;
	SkeletalMeshComponent->OverrideMaterials = OverrideMaterials;

	SkeletalMeshComponent->bPoseDirty = true;
	SkeletalMeshComponent->bNormalMapEnabled = bNormalMapEnabled;

	SkeletalMeshComponent->SkinnedVertices.SetNum(SkinnedVertices.Num());
	SkeletalMeshComponent->SkinningMatrices.SetNum(SkinningMatrices.Num());
	SkeletalMeshComponent->InvTransSkinningMatrices.SetNum(SkinningMatrices.Num());

	// VertexBuffer와 IndexBuffer는 새로 생성: CPU SKinning을 하므로.
	UStaticMesh* StaticMesh = SkeletalMeshAsset->GetStaticMesh();

	SkeletalMeshComponent->VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(
		StaticMesh->GetVertices().GetData(),
		static_cast<uint32>(StaticMesh->GetVertices().Num()) * sizeof(FNormalVertex),
		true
	);
	SkeletalMeshComponent->IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(
		StaticMesh->GetIndices().GetData(),
		static_cast<uint32>(StaticMesh->GetIndices().Num()) * sizeof(uint32)
	);

	// BoundingBox를 복사된 StaticMesh에 맞게 재설정
	// PrimitiveComponent::Duplicate()이 원본의 BoundingBox를 복사했지만,
	// 명시적으로 재설정하여 논리적 완결성 확보
	if (StaticMesh)
	{
		UAssetManager& AssetManager = UAssetManager::GetInstance();
		SkeletalMeshComponent->BoundingBox = &AssetManager.GetStaticMeshAABB(StaticMesh->GetAssetPathFileName());
	}

	return SkeletalMeshComponent;
}

void USkeletalMeshComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

void USkeletalMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FString AssetPath;
		FJsonSerializer::ReadString(InOutHandle, "SkeletalMeshAsset", AssetPath);

		if (!AssetPath.IsEmpty())
		{
			LoadSkeletalMeshAsset(AssetPath);
		}

		JSON OverrideMaterialJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "OverrideMaterial", OverrideMaterialJson, nullptr, false))
		{
			for (auto& Pair : OverrideMaterialJson.ObjectRange())
			{
				const FString& IdString = Pair.first;
				JSON& MaterialPathDataJson = Pair.second;

				int32 MaterialId;
				try { MaterialId = std::stoi(IdString); }
				catch (const std::exception&) { continue; }

				FString MaterialPath;
				FJsonSerializer::ReadString(MaterialPathDataJson, "Path", MaterialPath);

				for (TObjectIterator<UMaterial> It; It; ++It)
				{
					UMaterial* Mat = *It;
					if (!Mat) continue;
					if (Mat->GetDiffuseTexture() == nullptr)
					{
						continue;
					}

					FString DiffuseTexturePath = Mat->GetDiffuseTexture()->GetFilePath().ToString();
					if (DiffuseTexturePath == MaterialPath)
					{
						SetMaterial(MaterialId, Mat);
						break;
					}
				}
			}
		}

		FJsonSerializer::ReadBool(InOutHandle, "bNormalMapEnabled", bNormalMapEnabled, false);

		JSON BoneTransformsArray;
		if (FJsonSerializer::ReadArray(InOutHandle, "BoneSpaceTransforms", BoneTransformsArray, nullptr, false))
		{
			BoneSpaceTransforms.Empty();
			BoneSpaceTransforms.Reserve(BoneTransformsArray.size());

			for (size_t BoneIdx = 0; BoneIdx < BoneTransformsArray.size(); ++BoneIdx)
			{
				JSON& TransformJson = BoneTransformsArray[BoneIdx];
				FTransform Transform;

				FVector Location;
				FJsonSerializer::ReadVector(TransformJson, "Location", Location, FVector::Zero(), false);
				Transform.Translation = Location;

				FVector RotationEuler;
				FJsonSerializer::ReadVector(TransformJson, "Rotation", RotationEuler, FVector::ZeroVector(), false);
				Transform.Rotation = FQuaternion::FromEuler(RotationEuler);

				FVector Scale;
				FJsonSerializer::ReadVector(TransformJson, "Scale", Scale, FVector::One(), false);
				Transform.Scale = Scale;

				BoneSpaceTransforms.Add(Transform);
			}

			if (BoneSpaceTransforms.Num() > 0)
			{
				bPoseDirty = true;
			}
		}

		// 불러온 BoneSpaceTransforms에 맞춰 정점들 갱신
		RefreshBoneTransforms();
		UpdateSkinnedVertices();
	}
	else
	{
		if (SkeletalMeshAsset)
		{
			InOutHandle["SkeletalMeshAsset"] = SkeletalMeshAsset->GetStaticMesh()->GetAssetPathFileName().ToString();

			if (OverrideMaterials.Num() > 0)
			{
				JSON MaterialsJson = json::Object();
				for (int32 Idx = 0; Idx < OverrideMaterials.Num(); ++Idx)
				{
					const UMaterial* Material = OverrideMaterials[Idx];
					if (Material)
					{
						JSON MaterialJson;
						MaterialJson["Path"] = Material->GetDiffuseTexture()->GetFilePath().ToString();
						MaterialsJson[std::to_string(Idx)] = MaterialJson;
					}
				}
				InOutHandle["OverrideMaterial"] = MaterialsJson;
			}

			InOutHandle["bNormalMapEnabled"] = bNormalMapEnabled;

			if (BoneSpaceTransforms.Num() > 0)
			{
				JSON BoneTransformsArray = JSON::Make(JSON::Class::Array);
				for (int32 BoneIdx = 0; BoneIdx < BoneSpaceTransforms.Num(); ++BoneIdx)
				{
					const FTransform& Transform = BoneSpaceTransforms[BoneIdx];
					JSON TransformJson = json::Object();

					TransformJson["Location"] = FJsonSerializer::VectorToJson(Transform.Translation);
					TransformJson["Rotation"] = FJsonSerializer::VectorToJson(Transform.Rotation.ToEuler());
					TransformJson["Scale"] = FJsonSerializer::VectorToJson(Transform.Scale);

					BoneTransformsArray.append(TransformJson);
				}
				InOutHandle["BoneSpaceTransforms"] = BoneTransformsArray;
			}
		}
	}
}

void USkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	/** @todo */
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	UpdateSkinnedVertices();
}

void USkeletalMeshComponent::EndPlay()
{
	Super::EndPlay();
	/** @todo */
}

UClass* USkeletalMeshComponent::GetSpecificWidgetClass() const
{
	return USkeletalMeshComponentWidget::StaticClass();
}

void USkeletalMeshComponent::RefreshBoneTransforms()
{
	RefreshBoneTransformsCustom(BoneSpaceTransforms, bPoseDirty);
}

void USkeletalMeshComponent::RefreshBoneTransformsCustom(const TArray<FTransform>& InBoneSpaceTransforms, bool& InbPoseDirty)
{
	if (!GetSkeletalMeshAsset() || GetNumComponentSpaceTransforms() == 0)
	{
		return;
	}

	if (!InbPoseDirty)
	{
		return;
	}

	UE_LOG("USkeletalMeshComponent::RefreshBoneTransformsCustom - 본 트랜스폼 갱신 중...");

	/** @note LOD 시스템이 없으므로 모든 본을 사용한다. */
	TArray<FBoneIndexType> FillComponentSpaceTransformsRequiredBones(GetSkeletalMeshAsset()->GetRefSkeleton().GetRawBoneNum());
	std::iota(FillComponentSpaceTransformsRequiredBones.begin(), FillComponentSpaceTransformsRequiredBones.end(), 0);
	TArray<FTransform>& EditableSpaceBases = GetEditableComponentSpaceTransform();

	/** 현재는 애니메이션 시스템이 없으므로 FillComponentSpaceTransforms를 직접 호출한다. */
	SkeletalMeshAsset->FillComponentSpaceTransforms(
		InBoneSpaceTransforms,
		FillComponentSpaceTransformsRequiredBones,
		EditableSpaceBases
	);

	const TArray<FMatrix>& InvBindMatrices = SkeletalMeshAsset->GetRefBasesInvMatrix();

	for (int32 i = 0; i < FillComponentSpaceTransformsRequiredBones.Num(); ++i)
	{
		const int32 BoneIndex = FillComponentSpaceTransformsRequiredBones[i];

		/** 스키닝 행렬 = (모델 공간 -> 본 공간) * (본 공간 -> 포즈 모델 공간) */
		SkinningMatrices[BoneIndex] = InvBindMatrices[BoneIndex] * EditableSpaceBases[BoneIndex].ToMatrixWithScale();
		InvTransSkinningMatrices[BoneIndex] = SkinningMatrices[BoneIndex].Inverse().Transpose();
	}

	InbPoseDirty = false;
	bSkinningDirty = true;
}

void USkeletalMeshComponent::TickPose(float DeltaTime)
{
	Super::TickPose(DeltaTime);

	// @todo TickAnimation
}

USkeletalMesh* USkeletalMeshComponent::GetSkeletalMeshAsset() const
{
	return Cast<USkeletalMesh>(GetSkinnedAsset());
}

void USkeletalMeshComponent::SetSkeletalMeshAsset(USkeletalMesh* NewMesh)
{
	if (NewMesh == GetSkeletalMeshAsset())
	{
		// 이미 사용중인 에셋일 경우, 아무것도 하지않고 반환한다.
		return;
	}

	SafeRelease(VertexBuffer);
	SafeRelease(IndexBuffer);

	// 부모 클래스의 멤버(SkinnedAsset)에 에셋을 설정
	SetSkinnedAsset(NewMesh);
	SkeletalMeshAsset = NewMesh;

	if (SkeletalMeshAsset)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetRefSkeleton();
		UStaticMesh* StaticMesh = SkeletalMeshAsset->GetStaticMesh();

		const int32 NewNumBones = RefSkeleton.GetRawBoneNum();
		const int32 NewNumVertices = StaticMesh->GetVertices().Num();

		BoneSpaceTransforms = RefSkeleton.GetRawRefBonePose();
		SkinnedVertices.SetNum(NewNumVertices);
		SkinningMatrices.SetNum(NewNumBones);
		InvTransSkinningMatrices.SetNum(NewNumVertices);
		GetEditableComponentSpaceTransform().SetNum(NewNumBones);
		GetEditableBoneVisibilityStates().SetNum(NewNumBones);


		Vertices = &(StaticMesh->GetVertices());
		VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(
			StaticMesh->GetVertices().GetData(),
			static_cast<uint32>(StaticMesh->GetVertices().Num()) * sizeof(FNormalVertex),
			true
		);
		NumVertices = static_cast<uint32>(NewNumVertices);

		Indices = &(StaticMesh->GetIndices());
		IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(
			StaticMesh->GetIndices().GetData(),
			static_cast<uint32>(StaticMesh->GetIndices().Num()) * sizeof(uint32)
		);
		NumIndices = static_cast<uint32>(Indices->Num());

		bPoseDirty = true;
		bSkinningDirty = true;
	}
	else
	{
		BoneSpaceTransforms.Empty();
		SkinnedVertices.Empty();
		SkinningMatrices.Empty();
		InvTransSkinningMatrices.Empty();
		GetEditableComponentSpaceTransform().Empty();
		GetEditableBoneVisibilityStates().Empty();
	}
}

void USkeletalMeshComponent::LoadSkeletalMeshAsset(const FName& FilePath)
{
	USkeletalMesh* NewSkeletalMesh = FFbxManager::LoadFbxSkeletalMesh(FilePath);

	if (NewSkeletalMesh)
	{
		SetSkeletalMeshAsset(NewSkeletalMesh);
	}
}

FTransform USkeletalMeshComponent::GetBoneTransformLocal(int32 BoneIndex)
{
	return BoneSpaceTransforms[BoneIndex];
}

void USkeletalMeshComponent::SetBoneTransformLocal(int32 BoneIndex, const FTransform& NewLocalTransform)
{
	if (BoneSpaceTransforms.IsValidIndex(BoneIndex))
	{
		BoneSpaceTransforms[BoneIndex] = NewLocalTransform;
		bPoseDirty = true;
	}
}

UMaterial* USkeletalMeshComponent::GetMaterial(int32 Index) const
{
	if (OverrideMaterials.IsValidIndex(Index))
	{
		return OverrideMaterials[Index];
	}
	UStaticMesh* StaticMesh = SkeletalMeshAsset->GetStaticMesh();
	return StaticMesh ? StaticMesh->GetMaterial(Index) : nullptr;
}

void USkeletalMeshComponent::SetMaterial(int32 Index, UMaterial* InMaterial)
{
	if (Index < 0) return;

	if (Index >= OverrideMaterials.Num())
	{
		OverrideMaterials.SetNum(Index + 1, nullptr);
	}
	OverrideMaterials[Index] = InMaterial;
}

void USkeletalMeshComponent::UpdateSkinnedVertices()
{
	if (!bSkinningDirty)
	{
		return;
	}

	if (!SkeletalMeshAsset)
	{
		return;
	}

	FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetSkeletalMeshRenderData();
	const TArray<FNormalVertex>& Vertices = GetSkeletalMeshAsset()->GetStaticMesh()->GetVertices();
	const TArray<FRawSkinWeight>& SkinWeights = RenderData->SkinWeightVertices;

	for (int32 VertexIndex = 0; VertexIndex < Vertices.Num(); ++VertexIndex)
	{
		const FNormalVertex& Vertex = Vertices[VertexIndex];
		const FRawSkinWeight& SkinWeight = SkinWeights[VertexIndex];

		FVector FinalPosition(0.0f, 0.0f, 0.0f);
		FVector FinalNormal(0.0f, 0.0f, 0.0f);
		FVector FinalTangent(0.0f, 0.0f, 0.0f);

		uint32 TotalWeight = 0;
		for (int32 InfluenceIndex = 0; InfluenceIndex < FRawSkinWeight::MAX_TOTAL_INFLUENCES; ++InfluenceIndex)
		{
			const FBoneIndexType BoneIndex = SkinWeight.InfluenceBones[InfluenceIndex];
			if (BoneIndex == static_cast<FBoneIndexType>(-1))
			{
				continue;
			}
			const uint16 Weight = SkinWeight.InfluenceWeights[InfluenceIndex];
			TotalWeight += Weight;

			if (Weight == 0)
			{
				continue;
			}

			const FMatrix& FinalMatrix = SkinningMatrices[BoneIndex];
			const FMatrix& FinalInvTransMatrix = InvTransSkinningMatrices[BoneIndex];

			FinalPosition += FinalMatrix.TransformPosition(Vertex.Position) * Weight;
			FinalNormal += FinalInvTransMatrix.TransformVector(Vertex.Normal) * Weight;
			FinalTangent += FinalMatrix.TransformVector(FVector(Vertex.Tangent)) * Weight;
		}
		FinalPosition = FinalPosition / TotalWeight;

		FinalNormal = FinalNormal / TotalWeight;
		FinalNormal.Normalize();

		FinalTangent = FinalTangent / TotalWeight;
		FinalTangent = FinalTangent - (FinalNormal.Dot(FinalTangent)) * FinalNormal;
		FinalTangent.Normalize();

		FNormalVertex& ResultVertex = SkinnedVertices[VertexIndex];
		ResultVertex.Position = FinalPosition;
		ResultVertex.Normal = FinalNormal;
		ResultVertex.Tangent = FVector4(FinalTangent, Vertex.Tangent.W);
		ResultVertex.Color = Vertex.Color;
		ResultVertex.TexCoord = Vertex.TexCoord;
	}

	FRenderResourceFactory::UpdateVertexBufferData(VertexBuffer, SkinnedVertices);

	bSkinningDirty = false;
}

void USkeletalMeshComponent::EnableNormalMap()
{
	bNormalMapEnabled = true;
}

void USkeletalMeshComponent::DisableNormalMap()
{
	bNormalMapEnabled = false;
}

bool USkeletalMeshComponent::IsNormalMapEnabled() const
{
	return bNormalMapEnabled;
}
