#pragma once
#include "Component/Public/ActorComponent.h"

namespace json { class JSON; }
using JSON = json::JSON;

UCLASS()
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent();

	void BeginPlay() override;
	    void TickComponent(float DeltaTime) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	virtual void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FQuaternion& Rotation);
	void SetRelativeScale3D(const FVector& Scale);
	void SetUniformScale(bool bIsUniform);

	bool IsUniformScale() const;

	const FVector& GetRelativeLocation() const { return RelativeLocation; }
	const FQuaternion& GetRelativeRotation() const { return RelativeRotation; }
	const FVector& GetRelativeScale3D() const { return RelativeScale3D; }

	const FMatrix& GetWorldTransformMatrix() const;
	const FMatrix& GetWorldTransformMatrixInverse() const;

	FTransform GetWorldTransform() const;

	FVector GetWorldLocation() const;
    FVector GetWorldRotation() const;
    FQuaternion GetWorldRotationAsQuaternion() const;
    FVector GetWorldScale3D() const;

	FVector GetForwardVector() const;
	FVector GetUpVector() const;
	FVector GetRightVector() const;

    void SetWorldLocation(const FVector& NewLocation);
    void SetWorldRotation(const FVector& NewRotation);
    void SetWorldRotation(const FQuaternion& NewRotation);
    void SetWorldScale3D(const FVector& NewScale);

private:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
	mutable FMatrix WorldTransformMatrix;
	mutable FMatrix WorldTransformMatrixInverse;

	FVector RelativeLocation = FVector{ 0,0,0.f };
	FQuaternion RelativeRotation = FQuaternion::Identity();
	FVector RelativeScale3D = FVector{ 1.f,1.f,1.f };
	bool bIsUniformScale = false;

	bool bAbsoluteLocation = false;
	bool bAbsoluteRotation = false;
	bool bAbsoluteScale = false;

public:
	bool IsUsingAbsoluteLocation() const { return bAbsoluteLocation; }
	bool IsUsingAbsoluteRotation() const { return bAbsoluteRotation; }
	bool IsUsingAbsoluteScale() const { return bAbsoluteScale; }

	void SetAbsoluteLocation(bool bInAbsolute) { bAbsoluteLocation = bInAbsolute; MarkAsDirty(); }
	void SetAbsoluteRotation(bool bInAbsolute) { bAbsoluteRotation = bInAbsolute; MarkAsDirty(); }
	void SetAbsoluteScale(bool bInAbsolute) { bAbsoluteScale = bInAbsolute; MarkAsDirty(); }

	bool InheritYaw() const { return bInheritYaw; }
	bool InheritPitch() const { return bInheritPitch; }
	bool InheritRoll() const { return bInheritRoll; }

	void SetInheritYaw(bool InbInheritYaw) { bInheritYaw = InbInheritYaw; MarkAsDirty(); }
	void SetInheritPitch(bool InbInheritPitch) { bInheritPitch = InbInheritPitch; MarkAsDirty(); }
	void SetInheritRoll(bool InbInheritRoll) { bInheritRoll = InbInheritRoll; MarkAsDirty(); }

private:
	// 부모(캐릭터 루트)의 회전을 상속받을지 정하는 플래그
	bool bInheritYaw = true;
	bool bInheritPitch = true;
	bool bInheritRoll = true;

	// SceneComponent Hierarchy Section
public:
	USceneComponent* GetAttachParent() const { return AttachParent; }
	void AttachToComponent(USceneComponent* Parent, bool bRemainTransform = false);
	void DetachFromComponent();
	bool IsAttachedTo(const USceneComponent* Parent) const { return AttachParent == Parent; }
	const TArray<USceneComponent*>& GetChildren() const { return AttachChildren; }

protected:
	void DetachChild(USceneComponent* ChildToDetach);

private:
	USceneComponent* AttachParent = nullptr;
	TArray<USceneComponent*> AttachChildren;

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
};
