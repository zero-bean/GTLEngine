#include "pch.h"
#include "Component/Public/SceneComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
}

void USceneComponent::BeginPlay()
{

}

void USceneComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}
void USceneComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "Location", RelativeLocation, FVector::ZeroVector());
		FVector RotationEuler;
		FJsonSerializer::ReadVector(InOutHandle, "Rotation", RotationEuler, FVector::ZeroVector());
		RelativeRotation = FQuaternion::FromEuler(RotationEuler);

		FJsonSerializer::ReadVector(InOutHandle, "Scale", RelativeScale3D, FVector::OneVector());

		FJsonSerializer::ReadBool(InOutHandle, "AbsoluteLocation", bAbsoluteLocation, false);
		FJsonSerializer::ReadBool(InOutHandle, "AbsoluteRotation", bAbsoluteRotation, false);
		FJsonSerializer::ReadBool(InOutHandle, "AbsoluteScale", bAbsoluteScale, false);
	}
	// 저장
	else
	{
		InOutHandle["Location"] = FJsonSerializer::VectorToJson(RelativeLocation);
		InOutHandle["Rotation"] = FJsonSerializer::VectorToJson(RelativeRotation.ToEuler());
		InOutHandle["Scale"] = FJsonSerializer::VectorToJson(RelativeScale3D);

		InOutHandle["AbsoluteLocation"] = bAbsoluteLocation;
		InOutHandle["AbsoluteRotation"] = bAbsoluteRotation;
		InOutHandle["AbsoluteScale"] = bAbsoluteScale;
	}
}

void USceneComponent::AttachToComponent(USceneComponent* Parent, bool bRemainTransform)
{
	if (!Parent || Parent == this || GetOwner() != Parent->GetOwner()) { return; }
	if (AttachParent)
	{
		AttachParent->DetachChild(this);
	}

	AttachParent = Parent;
	Parent->AttachChildren.Emplace(this);

	MarkAsDirty();
}

void USceneComponent::DetachFromComponent()
{
	if (AttachParent)
	{
		AttachParent->DetachChild(this);
		AttachParent = nullptr;
	}
}

void USceneComponent::DetachChild(USceneComponent* ChildToDetach)
{
	AttachChildren.Remove(ChildToDetach);
}

UObject* USceneComponent::Duplicate()
{
	USceneComponent* SceneComponent = Cast<USceneComponent>(Super::Duplicate());
	SceneComponent->RelativeLocation = RelativeLocation;
	SceneComponent->RelativeRotation = RelativeRotation;
	SceneComponent->RelativeScale3D = RelativeScale3D;
	SceneComponent->MarkAsDirty();
	return SceneComponent;
}

void USceneComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	for (USceneComponent* Child : AttachChildren)
	{
		Child->MarkAsDirty();
	}
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
	// Note: PrimitiveComponent::MarkAsDirty() handles octree update and overlap checks
}

void USceneComponent::SetRelativeRotation(const FQuaternion& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
	// Note: PrimitiveComponent::MarkAsDirty() handles octree update and overlap checks
}

void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	RelativeScale3D = Scale;
	MarkAsDirty();
	// Note: PrimitiveComponent::MarkAsDirty() handles octree update and overlap checks
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
	if (bIsTransformDirty)
	{
		// Absolute 플래그에 따라 각 컴포넌트를 개별 처리
		FVector WorldLocation = RelativeLocation;
		FQuaternion WorldRotation = RelativeRotation;
		FVector WorldScale = RelativeScale3D;

		if (AttachParent)
		{
			// Location: Absolute가 아니면 부모 변환 적용
			if (!bAbsoluteLocation)
			{
				WorldLocation = AttachParent->GetWorldTransformMatrix().TransformPosition(RelativeLocation);
			}

			// Rotation: Absolute가 아니면 부모 회전 적용
			if (!bAbsoluteRotation)
			{
				FQuaternion ParentRotation = AttachParent->GetWorldRotationAsQuaternion();
				if (bInheritPitch && bInheritYaw && bInheritRoll)
				{
					WorldRotation = ParentRotation * RelativeRotation;
				}
				else // 하나라도 false이면, 필터링 로직 수행
				{
					FRotator ParentRotator = ParentRotation.ToRotator();

					// 상속을 원하지 않는 축을 0으로 초기화
					if (!bInheritPitch) ParentRotator.Pitch = 0.0f;
					if (!bInheritYaw)   ParentRotator.Yaw   = 0.0f;
					if (!bInheritRoll)  ParentRotator.Roll  = 0.0f;

					FQuaternion FilteredParentRotation = ParentRotator.Quaternion();
					WorldRotation = FilteredParentRotation * RelativeRotation;
				}
			}

			// Scale: Absolute가 아니면 부모 스케일 적용
			if (!bAbsoluteScale)
			{
				const FVector ParentScale = AttachParent->GetWorldScale3D();
				WorldScale = FVector(
					RelativeScale3D.X * ParentScale.X,
					RelativeScale3D.Y * ParentScale.Y,
					RelativeScale3D.Z * ParentScale.Z
				);
			}
		}

		WorldTransformMatrix = FMatrix::GetModelMatrix(WorldLocation, WorldRotation, WorldScale);
		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{
	if (bIsTransformDirtyInverse)
	{
		// World Transform의 역행렬이므로 GetWorldTransformMatrix의 역순으로 계산
		FVector WorldLocation = RelativeLocation;
		FQuaternion WorldRotation = RelativeRotation;
		FVector WorldScale = RelativeScale3D;

		if (AttachParent)
		{
			// Location: Absolute가 아니면 부모 변환 적용
			if (!bAbsoluteLocation)
			{
				WorldLocation = AttachParent->GetWorldTransformMatrix().TransformPosition(RelativeLocation);
			}

			// Rotation: Absolute가 아니면 부모 회전 적용
			if (!bAbsoluteRotation)
			{
				WorldRotation = AttachParent->GetWorldRotationAsQuaternion() * RelativeRotation;
			}

			// Scale: Absolute가 아니면 부모 스케일 적용
			if (!bAbsoluteScale)
			{
				const FVector ParentScale = AttachParent->GetWorldScale3D();
				WorldScale = FVector(
					RelativeScale3D.X * ParentScale.X,
					RelativeScale3D.Y * ParentScale.Y,
					RelativeScale3D.Z * ParentScale.Z
				);
			}
		}

		WorldTransformMatrixInverse = FMatrix::GetModelMatrixInverse(WorldLocation, WorldRotation, WorldScale);
		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
}

FTransform USceneComponent::GetWorldTransform() const
{
	FTransform WorldTransform;
	WorldTransform.Translation = GetWorldLocation();
	WorldTransform.Rotation = GetWorldRotationAsQuaternion();
	WorldTransform.Scale = GetWorldScale3D();
	return WorldTransform;
}

FVector USceneComponent::GetWorldLocation() const
{
    return GetWorldTransformMatrix().GetLocation();
}

FQuaternion USceneComponent::GetWorldRotationAsQuaternion() const
{
    if (AttachParent && !IsUsingAbsoluteRotation())
    {
        return AttachParent->GetWorldRotationAsQuaternion() * RelativeRotation;
    }
    // bAbsoluteRotation=true 또는 부모 없음: Relative가 곧 World
    return RelativeRotation;
}

FVector USceneComponent::GetWorldRotation() const
{
    return GetWorldRotationAsQuaternion().ToEuler();
}

FVector USceneComponent::GetWorldScale3D() const
{
    return GetWorldTransformMatrix().GetScale();
}

FVector USceneComponent::GetForwardVector() const
{
	return GetWorldRotationAsQuaternion().RotateVector(FVector::ForwardVector());
}

FVector USceneComponent::GetUpVector() const
{
	return GetWorldRotationAsQuaternion().RotateVector(FVector::UpVector());

}

FVector USceneComponent::GetRightVector() const
{
	return GetWorldRotationAsQuaternion().RotateVector(FVector::RightVector());
}

void USceneComponent::SetWorldLocation(const FVector& NewLocation)
{
    if (AttachParent && !IsUsingAbsoluteLocation())
    {
        const FMatrix ParentWorldMatrixInverse = AttachParent->GetWorldTransformMatrixInverse();
        SetRelativeLocation(ParentWorldMatrixInverse.TransformPosition(NewLocation));
    }
    else
    {
        SetRelativeLocation(NewLocation);
    }
}

void USceneComponent::SetWorldRotation(const FVector& NewRotation)
{
    FQuaternion NewWorldRotationQuat = FQuaternion::FromEuler(NewRotation);
    if (AttachParent && !IsUsingAbsoluteRotation())
    {
        FQuaternion ParentWorldRotationQuat = AttachParent->GetWorldRotationAsQuaternion();
        SetRelativeRotation(ParentWorldRotationQuat.Inverse() * NewWorldRotationQuat);
    }
    else
    {
        SetRelativeRotation(NewWorldRotationQuat);
    }
}

void USceneComponent::SetWorldRotation(const FQuaternion& NewRotation)
{
	if (AttachParent && !IsUsingAbsoluteRotation())
	{
		FQuaternion ParentWorldRotationQuat = AttachParent->GetWorldRotationAsQuaternion();
		SetRelativeRotation(ParentWorldRotationQuat.Inverse() * NewRotation);
	}
	else
	{
		SetRelativeRotation(NewRotation);
	}
}

void USceneComponent::SetWorldScale3D(const FVector& NewScale)
{
    if (AttachParent && !IsUsingAbsoluteScale())
    {
        const FVector ParentWorldScale = AttachParent->GetWorldScale3D();
        SetRelativeScale3D(FVector(NewScale.X / ParentWorldScale.X, NewScale.Y / ParentWorldScale.Y, NewScale.Z / ParentWorldScale.Z));
    }
    else
    {
        SetRelativeScale3D(NewScale);
    }
}
