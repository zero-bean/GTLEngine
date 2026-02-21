#pragma once
#include "pch.h"
#include "Engine.h"
#include "World.h"

UWorld* GWorld = nullptr;
UEngine* GEngine = nullptr;

UEngine::UEngine()
{
}

UEngine::~UEngine() {}

UWorld* UEngine::GetWorld()
{
    if (!WorldContexts.empty())
        return WorldContexts.back().World();
    return nullptr;
   // return GWorld;쓰지마세요
}

UWorld* UEngine::GetWorld(EWorldType InWorldType)
{
    for (auto& Context : WorldContexts)
    {
        if (Context.WorldType == InWorldType)
        {
            return Context.World();
        }
    }
    return nullptr;
}

UWorld* UEngine::GetActiveWorld()
{
    if (!WorldContexts.empty())
        return WorldContexts.back().World();
    return nullptr;
}

void UEngine::Tick(float DeltaSeconds)
{
 
   // GWorld->Tick(DeltaSeconds);        

}
void UEngine::SetWorld(UWorld* InWorld, EWorldType InType)
{
    // 1. 먼저 해당 타입의 컨텍스트가 이미 있는지 검사
    for (auto& Context : WorldContexts)
    {
        if (Context.WorldType == InType)
        {
            Context.SetWorld(InWorld, InType);
            if (InType != EWorldType::None)
            {
                GWorld = InWorld; // 현재 활성 월드 갱신
                GWorld->WorldType = InType;
            }
            return;
        }
    }

    // 2. 없으면 새 컨텍스트 추가
    FWorldContext NewCtx;
    NewCtx.SetWorld(InWorld, InType);
    WorldContexts.push_back(NewCtx);

    // 3. 현재 활성 월드로 설정
    if (InType != EWorldType::None)
    {
        GWorld = InWorld;
        GWorld->WorldType = InType;
    }
}
void UEngine::Render()
{
    //GWorld->Render();
}