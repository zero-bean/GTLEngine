#include "pch.h"
#include "Component/Public/SceneComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "Location", RelativeLocation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Rotation", RelativeRotation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Scale", RelativeScale3D, FVector::OneVector());
	}
	else
	{
		InOutHandle["Location"] = FJsonSerializer::VectorToJson(RelativeLocation);
		InOutHandle["Rotation"] = FJsonSerializer::VectorToJson(RelativeRotation);
		InOutHandle["Scale"] = FJsonSerializer::VectorToJson(RelativeScale3D);

		if (ParentAttachment)
		{
			InOutHandle["ParentName"] = ParentAttachment->GetName().ToString();
		}
	}
}

UObject* USceneComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<USceneComponent*>(Super::Duplicate(Parameters));

	DupObject->bIsTransformDirty = bIsTransformDirty;
	DupObject->bIsTransformDirtyInverse = bIsTransformDirtyInverse;

	DupObject->WorldTransformMatrix = WorldTransformMatrix;
	DupObject->WorldTransformMatrixInverse = WorldTransformMatrixInverse;

	DupObject->RelativeLocation = RelativeLocation;
	DupObject->RelativeRotation = RelativeRotation;
	DupObject->RelativeScale3D = RelativeScale3D;

	// @note: 에디터 변수 비활성화
	//DupObject->bIsUniformScale = bIsUniformScale;

	if (ParentAttachment != nullptr)
	{
		if (auto It = Parameters.DuplicationSeed.find(ParentAttachment); It != Parameters.DuplicationSeed.end())
		{
			DupObject->ParentAttachment = static_cast<USceneComponent*>(It->second);
		}
		else
		{
			/** @note: ParentAttachment의 Outer가 자신의 Outer로 설정된다. */
			/** @todo: Outer가 USceneComponent 계층 구조를 반영해야 하는가? */
			auto Params = InitStaticDuplicateObjectParams(ParentAttachment, DupObject->GetOuter(), FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupComponent = static_cast<USceneComponent*>(ParentAttachment->Duplicate(Params));
			DupObject->ParentAttachment = DupComponent;
		}
	}

	for (auto& Child : Children)
	{
		if (auto It = Parameters.DuplicationSeed.find(Child); It != Parameters.DuplicationSeed.end())
		{
			DupObject->Children.emplace_back(static_cast<USceneComponent*>(It->second));
		}
		else
		{
			/** @note: Child의 Outer가 자신의 Outer로 설정된다. */
			/** @todo: Outer가 USceneComponent 계층 구조를 반영해야 하는가? */
			auto Params = InitStaticDuplicateObjectParams(Child, DupObject->GetOuter(), FName::GetNone(), Parameters.DuplicationSeed, Parameters.CreatedObjects);
			auto DupComponent = static_cast<USceneComponent*>(Child->Duplicate(Params));
			DupObject->Children.emplace_back(DupComponent);
		}
	}

	return DupObject;
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	//부모의 조상중에 내 자식이 있으면 순환참조 -> 스택오버플로우 일어남.
	for (USceneComponent* Ancester = NewParent; Ancester; Ancester = Ancester->ParentAttachment)
	{
		if (Ancester == this) //조상중에 내 자식이 있다면 조상중에 내가 있을 것임.
			return;
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	ParentAttachment = NewParent;

	// 새로운 부모가 있다면, 그 부모의 자식 목록에 자신을 추가
	if (ParentAttachment)
	{
		ParentAttachment->AddChild(this);
	}

	MarkAsDirty();
}

void USceneComponent::AddChild(USceneComponent* ChildAdded)
{
	if (!ChildAdded)
	{
		return;
	}

	Children.push_back(ChildAdded);
}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	if (!ChildDeleted)
	{
		return;
	}

	Children.erase(std::remove(Children.begin(), Children.end(), ChildDeleted), Children.end());
	Children.Shrink();
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	FVector ActualScale = Scale;
	if (ActualScale.X < MinScale)
		ActualScale.X = MinScale;
	if (ActualScale.Y < MinScale)
		ActualScale.Y = MinScale;
	if (ActualScale.Z < MinScale)
		ActualScale.Z = MinScale;
	RelativeScale3D = ActualScale;
	MarkAsDirty();
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
	if (bIsTransformDirty)
	{
		WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);

		for (USceneComponent* Ancestor = ParentAttachment; Ancestor; Ancestor = Ancestor->ParentAttachment)
		{
			WorldTransformMatrix *= FMatrix::GetModelMatrix(Ancestor->RelativeLocation, FVector::GetDegreeToRadian(Ancestor->RelativeRotation), Ancestor->RelativeScale3D);
		}

		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{
	if (bIsTransformDirtyInverse || bIsTransformDirty)
	{
		const FMatrix& WorldMatrix = GetWorldTransformMatrix();
		WorldTransformMatrixInverse = WorldMatrix.Inverse();
		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
}

const TArray<USceneComponent*>& USceneComponent::GetChildren() const
{
	return Children;
}
