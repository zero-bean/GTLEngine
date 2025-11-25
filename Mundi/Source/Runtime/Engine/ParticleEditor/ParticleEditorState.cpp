#include "pch.h"
#include "ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"

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
