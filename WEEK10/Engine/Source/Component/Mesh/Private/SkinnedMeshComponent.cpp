#include "pch.h"

#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_ABSTRACT_CLASS(USkinnedMeshComponent, UMeshComponent)

UObject* USkinnedMeshComponent::Duplicate()
{
	USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(Super::Duplicate());
	SkinnedMeshComponent->SkinnedAsset = SkinnedAsset;
	SkinnedMeshComponent->BoneVisibilityStates = BoneVisibilityStates;
	SkinnedMeshComponent->ComponentSpaceTransformArray.SetNum(ComponentSpaceTransformArray.Num());

	return SkinnedMeshComponent;
}

void USkinnedMeshComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		JSON BoneVisibilityArray;
		if (FJsonSerializer::ReadArray(InOutHandle, "BoneVisibilityStates", BoneVisibilityArray, nullptr, false))
		{
			BoneVisibilityStates.Empty();
			BoneVisibilityStates.Reserve(BoneVisibilityArray.size());

			for (size_t i = 0; i < BoneVisibilityArray.size(); ++i)
			{
				const JSON& VisibilityStateJson = BoneVisibilityArray[i];
				if (VisibilityStateJson.JSONType() == JSON::Class::Integral)
				{
					int32 VisibilityState = VisibilityStateJson.ToInt();
					BoneVisibilityStates.Add(static_cast<uint8>(VisibilityState));
				}
				else
				{
					BoneVisibilityStates.Add(BVS_Visible);
				}
			}
		}
	}
	else
	{
		if (BoneVisibilityStates.Num() > 0)
		{
			JSON BoneVisibilityArray = JSON::Make(JSON::Class::Array);
			for (int32 i = 0; i < BoneVisibilityStates.Num(); ++i)
			{
				BoneVisibilityArray.append(static_cast<int32>(BoneVisibilityStates[i]));
			}
			InOutHandle["BoneVisibilityStates"] = BoneVisibilityArray;
		}
	}
}

void USkinnedMeshComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USkinnedMeshComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	TickPose(DeltaTime);

	RefreshBoneTransforms();
}

void USkinnedMeshComponent::EndPlay()
{
	Super::EndPlay();
}

void USkinnedMeshComponent::TickPose(float DeltaTime)
{
}

void USkinnedMeshComponent::HideBone(int32 BoneIndex)
{
	TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		EditableBoneVisibilityStates[BoneIndex] = BVS_ExplicitlyHidden;
		// RebuildVisibilityArray();
	}
}

void USkinnedMeshComponent::UnHideBone(int32 BoneIndex)
{
	TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		EditableBoneVisibilityStates[BoneIndex] = BVS_Visible;
		// RebuildVisiblityArray();
	}
}

bool USkinnedMeshComponent::IsBoneHidden(int32 BoneIndex)
{
	const TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		if (BoneIndex != INDEX_NONE)
		{
			return EditableBoneVisibilityStates[BoneIndex] != BVS_Visible;
		}
	}
	return false;
}
