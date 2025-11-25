#include "pch.h"
#include "ParticleEditorSections.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"

void FParticleEditorEmitterSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.50f, 0.35f, 0.8f));
    ImGui::Text("Emitters");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Particle System Emitters:");
    ImGui::Spacing();

    ImGui::BeginChild("EmitterList", ImVec2(0, 0), true);
    {
        if (!ActiveState || !ActiveState->HasParticleSystem())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("No particle system loaded.");
            ImGui::TextWrapped("Use 'File > Open' or 'File > New' to get started.");
            ImGui::PopStyleColor();
        }
        else
        {
            const int32 EmitterCount = ActiveState->GetEmitterCount();

            if (EmitterCount == 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("No emitters in the system.");
                ImGui::TextWrapped("Use 'Emitter > Add Sprite Emitter' to add emitters.");
                ImGui::PopStyleColor();
            }
            else
            {
                for (int32 i = 0; i < EmitterCount; ++i)
                {
                    UParticleEmitter* Emitter = ActiveState->CurrentParticleSystem->Emitters[i];
                    if (!Emitter)
                        continue;

                    const bool bIsSelected = (ActiveState->SelectedEmitterIndex == i);

                    // 선택된 항목은 하이라이트
                    if (bIsSelected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.6f, 0.4f, 1.0f));
                    }

                    char EmitterLabel[64];
                    sprintf_s(EmitterLabel, "Emitter %d", i);
                    if (ImGui::Selectable(EmitterLabel, bIsSelected, ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        ActiveState->SelectedEmitterIndex = i;
                    }

                    if (bIsSelected)
                    {
                        ImGui::PopStyleColor();
                    }

                    // 간단한 정보 표시
                    if (Emitter->LODLevels.Num() > 0)
                    {
                        ImGui::Indent(16.0f);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                        ImGui::Text("  LOD Levels: %d", Emitter->LODLevels.Num());

                        if (Emitter->LODLevels[0])
                        {
                            ImGui::Text("  Modules: %d", Emitter->LODLevels[0]->Modules.Num());
                        }
                        ImGui::PopStyleColor();
                        ImGui::Unindent(16.0f);
                    }

                    ImGui::Spacing();
                }
            }
        }
    }
    ImGui::EndChild();
}
