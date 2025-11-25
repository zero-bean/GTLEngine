#include "pch.h"
#include "ParticleEditorSections.h"

void FParticleEditorCurveSection::Draw(const FParticleEditorSectionContext& Context)
{
    (void)Context;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Curve Editor");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Property Curves:");
    ImGui::Spacing();

    ImGui::BeginChild("CurveEditorContent", ImVec2(0, 0), true);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Curve editor for particle properties over time.");
        ImGui::TextWrapped("Select a property from Details panel to edit its curve.");
        ImGui::PopStyleColor();

        // TODO: Implement the actual curve editor overlay
    }
    ImGui::EndChild();
}
