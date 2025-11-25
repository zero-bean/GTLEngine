#include "pch.h"
#include "ParticleEditorSections.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleEditorState.h"

void FParticleEditorViewportSection::Draw(const FParticleEditorSectionContext& Context)
{
    ParticleEditorState* ActiveState = Context.ActiveState;

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Viewport");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    // ActiveState가 있으면 저장된 배경색 사용, 없으면 기본값 사용
    ImVec4 bgColor = ActiveState ?
        ImVec4(ActiveState->ViewportBackgroundColor[0],
               ActiveState->ViewportBackgroundColor[1],
               ActiveState->ViewportBackgroundColor[2],
               ActiveState->ViewportBackgroundColor[3])
        : ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
    ImGui::BeginChild("ViewportContent", viewportSize, false, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::Text("Particle Preview Viewport");
        ImGui::Text("Size: %.0fx%.0f", viewportSize.x, viewportSize.y);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
