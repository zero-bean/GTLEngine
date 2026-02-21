#include "pch.h"
#include "EditorClipboard.h"
#include "Actor.h"
#include "World.h"

UEditorClipboard& UEditorClipboard::GetInstance()
{
    static UEditorClipboard Instance;
    return Instance;
}

void UEditorClipboard::Copy(const TArray<AActor*>& Actors)
{
    CopiedActors.clear();

    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            CopiedActors.push_back(Actor);
        }
    }

    if (!CopiedActors.empty())
    {
        UE_LOG("EditorClipboard: Copied %d actor(s)", CopiedActors.size());
    }
}

TArray<AActor*> UEditorClipboard::Paste(UWorld* World, const FVector& SpawnOffset)
{
    TArray<AActor*> PastedActors;

    if (!World)
    {
        UE_LOG("EditorClipboard: Cannot paste - World is null");
        return PastedActors;
    }

    if (CopiedActors.empty())
    {
        UE_LOG("EditorClipboard: Nothing to paste");
        return PastedActors;
    }

    // 복사된 액터들을 복제
    for (AActor* OriginalActor : CopiedActors)
    {
        // 원본 액터가 삭제되었는지 확인
        if (!OriginalActor)
        {
            continue;
        }

        // 액터 복제
        AActor* DuplicatedActor = Cast<AActor>(OriginalActor->Duplicate());
        if (!DuplicatedActor)
        {
            UE_LOG("EditorClipboard: Failed to duplicate actor");
            continue;
        }

        // 위치 오프셋 적용
        FVector NewLocation = OriginalActor->GetActorLocation() + SpawnOffset;
        DuplicatedActor->SetActorLocation(NewLocation);

        // 월드에 스폰
        World->SpawnActor(DuplicatedActor);
		// 고유 이름 생성 및 설정
       
        PastedActors.push_back(DuplicatedActor);
    }

    if (!PastedActors.empty())
    {
        UE_LOG("EditorClipboard: Pasted %d actor(s)", PastedActors.size());
    }

    return PastedActors;
}

bool UEditorClipboard::HasCopiedActors() const
{
    return !CopiedActors.empty();
}

void UEditorClipboard::Clear()
{
    CopiedActors.clear();
    UE_LOG("EditorClipboard: Clipboard cleared");
}
