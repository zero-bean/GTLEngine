#pragma once
#include"pch.h"
#include "GameEngine.h"
#include "World.h"

UGameEngine::UGameEngine()
{
}

void UGameEngine::Tick(float DeltaSeconds)
{
    if (GameWorld)
        GameWorld->Tick(DeltaSeconds);
}

void UGameEngine::Render()
{
    // Renderer가 렌더링을 직접 담당 (World는 데이터만 제공)
    if (GameWorld)
    {
        if (URenderer* Renderer = GameWorld->GetRenderer())
        {
            Renderer->RenderFrame(GameWorld);
        }
    }
}

void UGameEngine::StartGame(UWorld* World)
{
    GameWorld = World;
    if (GameWorld)
    {
        // 모든 액터에 BeginPlay 초기화 호출
        GameWorld->InitializeActorsForPlay();
    }
}

void UGameEngine::EndGame()
{
    if (GameWorld)
    {
        GameWorld->CleanupWorld();
        // PIE 월드는 EditorEngine::EndPIE에서 삭제하므로 여기서는 nullptr만 설정
        DeleteObject(GameWorld);
        GameWorld = nullptr;
    }
}

UGameEngine::~UGameEngine()
{
}
