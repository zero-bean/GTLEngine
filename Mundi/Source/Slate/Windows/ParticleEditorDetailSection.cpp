#include "pch.h"
#include "ParticleEditorSections.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule/ParticleModule.h"
#include "ParticleModule/ParticleModuleRequired.h"
#include "ParticleModule/ParticleModuleSpawn.h"
#include "ParticleModule/ParticleModuleColor.h"
#include "ParticleModule/ParticleModuleSize.h"
#include "ParticleModule/ParticleModuleLifetime.h"
#include "ParticleModule/ParticleModuleLocation.h"
#include "ParticleModule/ParticleModuleVelocity.h"
#include "ParticleModule/ParticleModuleRotation.h"

void FParticleEditorDetailSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.50f, 0.35f, 0.25f, 0.8f));
    ImGui::Text("Details");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginChild("DetailContent", ImVec2(0, 0), true);

    // Early exit: No particle system
    if (!ActiveState || !ActiveState->HasParticleSystem())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No particle system loaded.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    // Early exit: No emitter selected
    UParticleEmitter* SelectedEmitter = ActiveState->GetSelectedEmitter();
    if (!SelectedEmitter)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an emitter to edit its properties.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    // Emitter header
    ImGui::Text("Emitter %d Properties", ActiveState->SelectedEmitterIndex);
    ImGui::Separator();
    ImGui::Spacing();

    // Get active LOD level (기본 LOD 0만 사용)
    if (SelectedEmitter->LODLevels.Num() == 0 || !SelectedEmitter->LODLevels[0])
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No LOD levels available for this emitter.");
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }

    UParticleLODLevel* LODLevel = SelectedEmitter->LODLevels[0];

    // Draw Required Module
    if (LODLevel->RequiredModule)
    {
        DrawRequiredModule(LODLevel->RequiredModule);
    }

    // Draw Spawn Module
    if (LODLevel->SpawnModule)
    {
        DrawSpawnModule(LODLevel->SpawnModule);
    }

    // Draw other modules
    for (int32 ModIdx = 0; ModIdx < LODLevel->Modules.Num(); ++ModIdx)
    {
        UParticleModule* Module = LODLevel->Modules[ModIdx];
        if (Module)
        {
            DrawModuleProperties(Module, ModIdx);
        }
    }

    ImGui::EndChild();
}

void FParticleEditorDetailSection::DrawRequiredModule(UParticleModuleRequired* Module)
{
    if (!ImGui::CollapsingHeader("Required Module", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Indent(16.0f);
    ImGui::Checkbox("Enabled##Required", &Module->bEnabled);
    ImGui::DragFloat("Emitter Duration", &Module->EmitterDuration, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Emitter Delay", &Module->EmitterDelay, 0.1f, 0.0f, 100.0f);
    ImGui::DragInt("Emitter Loops", &Module->EmitterLoops, 1, 0, 100);
    ImGui::Checkbox("Use Local Space", &Module->bUseLocalSpace);
    ImGui::Unindent(16.0f);
    ImGui::Spacing();
}

void FParticleEditorDetailSection::DrawSpawnModule(UParticleModuleSpawn* Module)
{
    if (!ImGui::CollapsingHeader("Spawn Module", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Indent(16.0f);
    ImGui::Checkbox("Enabled##Spawn", &Module->bEnabled);
    DrawDistributionFloat("Spawn Rate", Module->SpawnRate, 0.0f, 1000.0f);
    ImGui::Unindent(16.0f);
    ImGui::Spacing();
}

void FParticleEditorDetailSection::DrawModuleProperties(UParticleModule* Module, int32 ModuleIndex)
{
    const char* ClassName = Module->GetClass()->Name;
    char HeaderName[128];
    sprintf_s(HeaderName, "%s##%d", ClassName, ModuleIndex);

    if (!ImGui::CollapsingHeader(HeaderName))
        return;

    ImGui::Indent(16.0f);

    char EnabledID[64];
    sprintf_s(EnabledID, "Enabled##%d", ModuleIndex);
    ImGui::Checkbox(EnabledID, &Module->bEnabled);

    // 모듈 타입별 프로퍼티
    if (UParticleModuleLifetime* LifetimeMod = dynamic_cast<UParticleModuleLifetime*>(Module))
    {
        DrawDistributionFloat("Lifetime", LifetimeMod->LifeTime, 0.0f, 100.0f);
    }
    else if (UParticleModuleSize* SizeMod = dynamic_cast<UParticleModuleSize*>(Module))
    {
        DrawDistributionVector("Start Size", SizeMod->StartSize, 0.0f, 100.0f, false);
    }
    else if (UParticleModuleColor* ColorMod = dynamic_cast<UParticleModuleColor*>(Module))
    {
        DrawDistributionVector("Start Color", ColorMod->StartColor, 0.0f, 1.0f, true);
    }
    else if (UParticleModuleLocation* LocMod = dynamic_cast<UParticleModuleLocation*>(Module))
    {
        DrawDistributionVector("Spawn Location", LocMod->SpawnLocation, -1000.0f, 1000.0f, false);
    }
    else if (UParticleModuleVelocity* VelMod = dynamic_cast<UParticleModuleVelocity*>(Module))
    {
        DrawDistributionVector("Start Velocity", VelMod->StartVelocity, -1000.0f, 1000.0f, false);
    }
    else if (UParticleModuleRotation* RotMod = dynamic_cast<UParticleModuleRotation*>(Module))
    {
        DrawDistributionFloat("Start Rotation (Degrees)", RotMod->StartRotation, -360.0f, 360.0f);
    }

    ImGui::Unindent(16.0f);
    ImGui::Spacing();
}

void FParticleEditorDetailSection::DrawDistributionFloat(const char* Label, FRawDistributionFloat& Distribution, float Min, float Max)
{
    const char* DistModes[] = { "Constant", "Uniform", "Curve" };
    char ComboLabel[128];
    sprintf_s(ComboLabel, "%s Mode", Label);

    int32 CurrentMode = static_cast<int32>(Distribution.Operation);
    if (ImGui::Combo(ComboLabel, &CurrentMode, DistModes, IM_ARRAYSIZE(DistModes)))
    {
        Distribution.Operation = static_cast<EDistributionMode>(CurrentMode);
    }

    if (Distribution.Operation == EDistributionMode::DOP_Constant)
    {
        ImGui::DragFloat(Label, &Distribution.Constant, 0.1f, Min, Max);
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Uniform)
    {
        char MinLabel[128], MaxLabel[128];
        sprintf_s(MinLabel, "Min %s", Label);
        sprintf_s(MaxLabel, "Max %s", Label);
        ImGui::DragFloat(MinLabel, &Distribution.Min, 0.1f, Min, Max);
        ImGui::DragFloat(MaxLabel, &Distribution.Max, 0.1f, Min, Max);
    }
}

void FParticleEditorDetailSection::DrawDistributionVector(const char* Label, FRawDistributionVector& Distribution, float Min, float Max, bool bIsColor)
{
    const char* DistModes[] = { "Constant", "Uniform", "Curve" };
    char ComboLabel[128];
    sprintf_s(ComboLabel, "%s Mode", Label);

    int32 CurrentMode = static_cast<int32>(Distribution.Operation);
    if (ImGui::Combo(ComboLabel, &CurrentMode, DistModes, IM_ARRAYSIZE(DistModes)))
    {
        Distribution.Operation = static_cast<EDistributionMode>(CurrentMode);
    }

    if (Distribution.Operation == EDistributionMode::DOP_Constant)
    {
        if (bIsColor)
        {
            ImGui::ColorEdit3(Label, &Distribution.Constant.X);
        }
        else
        {
            ImGui::DragFloat3(Label, &Distribution.Constant.X, 0.1f, Min, Max);
        }
    }
    else if (Distribution.Operation == EDistributionMode::DOP_Uniform)
    {
        char MinLabel[128], MaxLabel[128];
        sprintf_s(MinLabel, "Min %s", Label);
        sprintf_s(MaxLabel, "Max %s", Label);

        if (bIsColor)
        {
            ImGui::ColorEdit3(MinLabel, &Distribution.Min.X);
            ImGui::ColorEdit3(MaxLabel, &Distribution.Max.X);
        }
        else
        {
            ImGui::DragFloat3(MinLabel, &Distribution.Min.X, 0.1f, Min, Max);
            ImGui::DragFloat3(MaxLabel, &Distribution.Max.X, 0.1f, Min, Max);
        }
    }
}
