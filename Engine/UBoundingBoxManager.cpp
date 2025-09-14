#pragma once
#include "stdafx.h"
#include "UBoundingBoxManager.h"
#include "UBoundingBoxComponent.h"
#include "UMeshManager.h"
#include "URenderer.h"
#include "UPrimitiveComponent.h"

IMPLEMENT_UCLASS(UBoundingBoxManager, UEngineSubsystem)

UBoundingBoxManager::UBoundingBoxManager() {}
UBoundingBoxManager::~UBoundingBoxManager()
{
    delete AABBComp;
    AABBComp = nullptr;
    Target = nullptr;
}

bool UBoundingBoxManager::Initialize(UMeshManager* InMeshManager)
{
    MeshManager = InMeshManager;
    AABBComp = NewObject<UBoundingBoxComponent>();
    if (!AABBComp->Init(InMeshManager)) {
        delete AABBComp; AABBComp = nullptr;
        return false;
    }
    // 기본 스타일
    AABBComp->SetColor({ 1,1,0,1 });
    AABBComp->SetDrawOnTop(true);
    return true;
}

void UBoundingBoxManager::SetTarget(UPrimitiveComponent* InTarget)
{
    Target = InTarget;
    if (AABBComp) AABBComp->SetTarget(Target);
}

void UBoundingBoxManager::UseExplicitLocalAABB(const FBoundingBox& Box)
{
    if (!AABBComp) return;
    AABBComp->SetSource(EAABBSource::Explicit);
    AABBComp->SetLocalBox(Box);
}

void UBoundingBoxManager::UseMeshLocalAABB(UMesh* MeshWithAABB)
{
    if (!AABBComp) return;
    AABBComp->SetSource(EAABBSource::FromMesh);
    AABBComp->SetTargetMesh(MeshWithAABB);
}

void UBoundingBoxManager::Update(float DeltaTime)
{
    if (!bEnabled || !AABBComp || !Target) return;
    AABBComp->Update(DeltaTime);
}

void UBoundingBoxManager::Draw(URenderer& Renderer)
{
    if (!bEnabled || !AABBComp || !Target) return;
    // 다른 기즈모처럼 항상 OnTop로 그리려면 DrawOnTop 사용
    AABBComp->Draw(Renderer);
}
