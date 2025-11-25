#include "pch.h"
#include "ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "Actor.h"
#include "World.h"
#include "ParticleSystemComponent.h"

UParticleEmitter* ParticleEditorState::GetSelectedEmitter() const
{
    if (!CurrentParticleSystem || SelectedEmitterIndex < 0)
        return nullptr;

    if (SelectedEmitterIndex >= CurrentParticleSystem->Emitters.Num())
        return nullptr;

    return CurrentParticleSystem->Emitters[SelectedEmitterIndex];
}

int32 ParticleEditorState::GetEmitterCount() const
{
    if (!CurrentParticleSystem)
        return 0;

    return CurrentParticleSystem->Emitters.Num();
}

void ParticleEditorState::CreatePreviewActor()
{
    if (!World)
        return;

    // 기존 프리뷰 액터가 있으면 파괴
    DestroyPreviewActor();

    // 새 액터 생성
    PreviewActor = World->SpawnActor<AActor>();
    if (!PreviewActor)
        return;

    // 에디터 프리뷰 월드에서도 틱이 동작하도록 설정
    PreviewActor->SetTickInEditor(true);

    // 파티클 시스템 컴포넌트 생성 및 부착
    PreviewComponent = static_cast<UParticleSystemComponent*>(
        PreviewActor->AddNewComponent(UParticleSystemComponent::StaticClass()));

    if (PreviewComponent)
    {
        PreviewActor->SetRootComponent(PreviewComponent);

        // 현재 파티클 시스템이 있으면 설정 (BeginPlay 전에 설정)
        if (CurrentParticleSystem)
        {
            PreviewComponent->Template = CurrentParticleSystem;
        }

        // 컴포넌트 등록 및 BeginPlay 호출
        PreviewActor->RegisterAllComponents(World);
        PreviewComponent->BeginPlay();
    }
}

void ParticleEditorState::DestroyPreviewActor()
{
    // World가 소멸될 때 자동으로 모든 액터를 정리하므로
    // 여기서는 포인터만 null로 설정
    // (World 소멸 전에 액터를 명시적으로 삭제하면 크래시 발생)
    PreviewActor = nullptr;
    PreviewComponent = nullptr;
}

void ParticleEditorState::UpdatePreviewParticleSystem()
{
    if (!PreviewComponent)
    {
        // 프리뷰 컴포넌트가 없으면 생성
        CreatePreviewActor();
        return;
    }

    // 파티클 시스템 템플릿 업데이트
    if (CurrentParticleSystem)
    {
        // EndPlay 호출 후 SetTemplate, 그리고 다시 BeginPlay
        PreviewComponent->EndPlay();
        PreviewComponent->SetTemplate(CurrentParticleSystem);
        PreviewComponent->BeginPlay();
    }
}
