// DecalSpotLightComponent.cpp (Inheritance Version)
#include "pch.h"
#include "DecalSpotLightComponent.h"
#include "ResourceManager.h"

IMPLEMENT_CLASS(UDecalSpotLightComponent)

UDecalSpotLightComponent::UDecalSpotLightComponent()
{
    bCanEverTick = false;
    // The parent UDecalComponent constructor has already run and handled:
    // - Loading the default DecalBoxMesh
    // - Setting the default TexturePath
    // - Initializing LocalAABB, etc.

    // We just need to override the parts that are different for a spotlight.

    // 1. Use the new spotlight shader instead of the default decal shader.
    SetMaterial("DecalSpotLightShader.hlsl");

    // 3. Call our overridden update function to create the initial perspective matrix.
    UpdateDecalProjectionMatrix();
    
}

void UDecalSpotLightComponent::UpdateDecalProjectionMatrix()
{
    FOBB WorldOBB = GetWorldOBB();

    // 1. Calculate and update member variables from the OBB extents
    Radius = WorldOBB.Extents.X;
    Height = WorldOBB.Extents.Y * 2.0f;
    Near = 1e-2f;
    Far = WorldOBB.Extents.Z;

    if (Height < KINDA_SMALL_NUMBER) return;

    Fov = atanf(Radius / Height) * 2.0f; // radians
    Aspect = (WorldOBB.Extents.Y > KINDA_SMALL_NUMBER) ? WorldOBB.Extents.X / WorldOBB.Extents.Y : 1.0f;

    // 2. Create the projection matrix using the member variables
    DecalProjectionMatrix = FMatrix::PerspectiveFovLH(Fov, Aspect, Near, Far);
}

const FVector4 UDecalSpotLightComponent::GetBoundingBoxColor() const
{
    return FVector4(1, 0, 0, 1);
}
const TArray<FVector> UDecalSpotLightComponent::GetBoundingBoxLines() const
{
    // SpotLight 절두체 그리기

    float halfFov = tanf(Fov / 2.0f);
    float nearHalfH = Near * halfFov;
    float nearHalfW = nearHalfH * Aspect;
    float farHalfH = Far * halfFov;
    float farHalfW = farHalfH * Aspect;

    FVector Corners[8];
    Corners[0] = FVector(nearHalfW, nearHalfH, Near);
    Corners[1] = FVector(-nearHalfW, nearHalfH, Near);
    Corners[2] = FVector(-nearHalfW, -nearHalfH, Near);
    Corners[3] = FVector(nearHalfW, -nearHalfH, Near);
    Corners[4] = FVector(farHalfW, farHalfH, Far);
    Corners[5] = FVector(-farHalfW, farHalfH, Far);
    Corners[6] = FVector(-farHalfW, -farHalfH, Far);
    Corners[7] = FVector(farHalfW, -farHalfH, Far);

    FMatrix WorldMatrix = GetWorldMatrix();
    for (int i = 0; i < 8; ++i)
    {
        FVector4 Transformed = WorldMatrix.TransformPosition(FVector4(Corners[i], 1.0f));
        Corners[i] = FVector(Transformed.X, Transformed.Y, Transformed.Z);
    }

    TArray<FVector> BoxWireLines =
    {
    Corners[0], Corners[1],
    Corners[1], Corners[2],
    Corners[2], Corners[3],
    Corners[3], Corners[0],
    Corners[4], Corners[5],
    Corners[5], Corners[6],
    Corners[6], Corners[7],
    Corners[7], Corners[4],
    Corners[0], Corners[4],
    Corners[1], Corners[5],
    Corners[2], Corners[6],
    Corners[3], Corners[7],
    };
    return BoxWireLines;
}


UObject* UDecalSpotLightComponent::Duplicate()
{
    UDecalSpotLightComponent* DuplicatedComponent = Cast<UDecalSpotLightComponent>(UDecalComponent::Duplicate());
    if (DuplicatedComponent)
    {
        DuplicatedComponent->TexturePath = TexturePath;
        //DuplicatedComponent->LocalAABB = LocalAABB;
        // DecalSpotLightComponent 특유의 프로퍼티 복사
        DuplicatedComponent->Radius = Radius;
        DuplicatedComponent->Height = Height;
        DuplicatedComponent->Near = Near;
        DuplicatedComponent->Far = Far;
        DuplicatedComponent->Fov = Fov;
        DuplicatedComponent->Aspect = Aspect;
    }
    return DuplicatedComponent;
}

void UDecalSpotLightComponent::DuplicateSubObjects()
{
    UDecalComponent::DuplicateSubObjects();
}

// Note: All other methods like Tick, Render, SetDecalTexture, etc.,
// are automatically inherited from UDecalComponent.