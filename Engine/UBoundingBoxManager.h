#pragma once
#include "stdafx.h"
#include "UEngineSubsystem.h"
#include "UClass.h"
#include "BoundingBox.h"
#include "UBoundingBoxComponent.h"

class UMeshManager;
class URenderer;
class UPrimitiveComponent;

class UBoundingBoxManager : public UEngineSubsystem
{
    DECLARE_UCLASS(UBoundingBoxManager, UEngineSubsystem)
public:
    UBoundingBoxManager();
    ~UBoundingBoxManager();

    bool Initialize(UMeshManager* meshManager);

    void SetTarget(UPrimitiveComponent* target);
    UPrimitiveComponent* GetTarget() const { return Target; }

    void SetEnabled(bool b) { bEnabled = b; }
    bool IsEnabled() const { return bEnabled; }

    // 로컬 AABB 소스 제어
    void UseExplicitLocalAABB(const FBoundingBox& b);
    void UseMeshLocalAABB(UMesh* meshWithAABB);

    // 프레임 루프
    void Update(float deltaTime);
    void Draw(URenderer& renderer);

private:
    UMeshManager* MeshManager = nullptr;
    UBoundingBoxComponent* AABBComp = nullptr;
    UPrimitiveComponent* Target = nullptr;
    bool bEnabled = true;
};
