// USceneComponent.cpp
#include "stdafx.h"
#include "USceneComponent.h"
#include "json.hpp"
#include "UClass.h"

IMPLEMENT_UCLASS(USceneComponent, UObject)
FMatrix USceneComponent::GetWorldTransform()
{
    return FMatrix::SRTRowQuaternion(RelativeLocation, RelativeQuaternion.ToMatrixRow(), RelativeScale3D);
}

json::JSON USceneComponent::Serialize() const
{
    FVector tmpRot = RelativeQuaternion.GetEulerXYZ();
    json::JSON result;
    result["Location"] = json::Array(RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
    result["Rotation"] = json::Array( tmpRot.X, tmpRot.Y, tmpRot.Z);
    result["Scale"] = json::Array(RelativeScale3D.X, RelativeScale3D.Y, RelativeScale3D.Z);
    result["Type"] = GetClass()->GetDisplayName();
	result["Name"] = Name.ToString();
    return result;
}

// -------------------------
// Deserialize
// -------------------------
bool USceneComponent::Deserialize(const json::JSON& data)
{
    UObject::Deserialize(data);

    if (!data.hasKey("Location") || !data.hasKey("Rotation") || !data.hasKey("Scale"))
        return false;

    auto loc = data.at("Location");
    if (loc.size() != 3) return false;
    RelativeLocation = FVector(static_cast<float>(loc[0].ToFloat()), static_cast<float>(loc[1].ToFloat()), static_cast<float>(loc[2].ToFloat()));

    auto rot = data.at("Rotation");
    if (rot.size() != 3) return false;
    RelativeQuaternion = FQuaternion::FromEulerXYZ(static_cast<float>(rot[0].ToFloat()), static_cast<float>(rot[1].ToFloat()), static_cast<float>(rot[2].ToFloat()));

    auto scale = data.at("Scale");
    if (scale.size() != 3) return false;
    RelativeScale3D = FVector(static_cast<float>(scale[0].ToFloat()), static_cast<float>(scale[1].ToFloat()), static_cast<float>(scale[2].ToFloat()));

    return true;
}
FVector USceneComponent::GetWorldLocation()
{
    const FMatrix M = GetWorldTransform();
    return FVector(M.M[3][0], M.M[3][1], M.M[3][2]); // row-vector 규약: translation = 4번째 행
}