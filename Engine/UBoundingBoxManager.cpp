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

bool UBoundingBoxManager::Initialize(UMeshManager* meshManager)
{
    MeshManager = meshManager;
    AABBComp = new UBoundingBoxComponent();
    if (!AABBComp->Init(meshManager)) {
        delete AABBComp; AABBComp = nullptr;
        return false;
    }
    // 기본 스타일
    AABBComp->SetColor({ 1,1,0,1 });
    AABBComp->SetDrawOnTop(true);
    return true;
}

void UBoundingBoxManager::SetTarget(UPrimitiveComponent* target)
{
    Target = target;
    if (AABBComp) AABBComp->SetTarget(Target);
}

void UBoundingBoxManager::UseExplicitLocalAABB(const FBoundingBox& b)
{
    if (!AABBComp) return;
    AABBComp->SetSource(EAABBSource::Explicit);
    AABBComp->SetLocalBox(b);
}

void UBoundingBoxManager::UseMeshLocalAABB(UMesh* meshWithAABB)
{
    if (!AABBComp) return;
    AABBComp->SetSource(EAABBSource::FromMesh);
    AABBComp->SetTargetMesh(meshWithAABB);
}

void UBoundingBoxManager::Update(float deltaTime)
{
    if (!bEnabled || !AABBComp || !Target) return;
    AABBComp->Update(deltaTime);
}

void UBoundingBoxManager::Draw(URenderer& renderer)
{
    if (!bEnabled || !AABBComp || !Target) return;
    // 다른 기즈모처럼 항상 OnTop로 그리려면 DrawOnTop 사용
    AABBComp->DrawOnTop(renderer);
}
