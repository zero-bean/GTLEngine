#include "pch.h"
#include "PrimitiveComponent.h"
#include "SceneLoader.h"
#include "SceneComponent.h"
#include "SceneRotationUtils.h"

void UPrimitiveComponent::SetMaterial(const FString& FilePath)
{
    Material = UResourceManager::GetInstance().Load<UMaterial>(FilePath);
}

void UPrimitiveComponent::Serialize(bool bIsLoading, FPrimitiveData& InOut)
{
    if (bIsLoading)
    {
        // FPrimitiveData -> 컴포넌트 월드 트랜스폼
        FTransform WT = GetWorldTransform();
        WT.Translation = InOut.Location;
        WT.Rotation = SceneRotUtil::QuatFromEulerZYX_Deg(InOut.Rotation);
        WT.Scale3D = InOut.Scale;
        SetWorldTransform(WT);
    }
    else
    {
        // 컴포넌트 월드 트랜스폼 -> FPrimitiveData
        const FTransform WT = GetWorldTransform();
        InOut.Location = WT.Translation;
        InOut.Rotation = SceneRotUtil::EulerZYX_Deg_FromQuat(WT.Rotation);
        InOut.Scale = WT.Scale3D;
    }
}

void UPrimitiveComponent::Serialize(bool bIsLoading, FComponentData& InOut)
{
    if (bIsLoading)
    {
        // FComponentData -> 컴포넌트 상대 트랜스폼
        SetRelativeLocation(InOut.RelativeLocation);
        SetRelativeRotation(FQuat::MakeFromEuler(InOut.RelativeRotation));
        SetRelativeScale(InOut.RelativeScale);
    }
    else
    {
        // 컴포넌트 상대 트랜스폼 -> FComponentData
        InOut.RelativeLocation = GetRelativeLocation();
        InOut.RelativeRotation = GetRelativeRotation().ToEuler();
        InOut.RelativeScale = GetRelativeScale();
    }
}
const FAABB UPrimitiveComponent::GetWorldAABB() const
{
    return FAABB();
}
const TArray<FVector> UPrimitiveComponent::GetBoundingBoxLines() const
{
    return GetWorldAABB().GetWireLine();
}
const FVector4 UPrimitiveComponent::GetBoundingBoxColor() const
{
    return FVector4(1, 1, 0, 1);
}
UObject* UPrimitiveComponent::Duplicate()
{
    UPrimitiveComponent* DuplicatedComponent = NewObject<UPrimitiveComponent>(*this);

    // Transform 속성 복사 (부모 속성)
    DuplicatedComponent->RelativeLocation = this->RelativeLocation;
    DuplicatedComponent->RelativeRotation = this->RelativeRotation;
    DuplicatedComponent->RelativeScale = this->RelativeScale;
    DuplicatedComponent->UpdateRelativeTransform();

    // PrimitiveComponent 속성 복사
    DuplicatedComponent->Material = this->Material;

    DuplicatedComponent->DuplicateSubObjects();

    return DuplicatedComponent;
}

void UPrimitiveComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();


}
